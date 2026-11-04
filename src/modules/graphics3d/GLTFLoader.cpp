/**
 * LOVING3D - glTF 2.0 / GLB model importer.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org). The underlying engine
 * remains freely distributable under the terms of its original zlib license
 * (see license.txt).
 **/

#include "GLTFLoader.h"

#include "common/Exception.h"
#include "Matrix4.h"
#include "Quaternion.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/Texture.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <limits>
#include <tuple>

namespace love
{
namespace graphics3d
{

namespace
{

struct Json
{
	enum Kind { NIL, BOOLEAN, NUMBER, STRING, ARRAY, OBJECT } kind = NIL;
	bool boolean = false;
	double number = 0.0;
	std::string string;
	std::vector<Json> array;
	std::map<std::string, Json> object;

	const Json &get(const char *key) const
	{
		static const Json missing;
		auto it = object.find(key);
		return it == object.end() ? missing : it->second;
	}

	double num(double fallback = 0.0) const { return kind == NUMBER ? number : fallback; }
	int integer(int fallback = 0) const { return (int) num(fallback); }
	const Json &at(size_t i) const { return i < array.size() ? array[i] : missingValue(); }
	static const Json &missingValue() { static const Json missing; return missing; }
};

class JsonParser
{
public:
	explicit JsonParser(const std::string &text) : source(text) {}

	Json parse()
	{
		Json value = readValue();
		space();
		if (position != source.size())
			fail("trailing data");
		return value;
	}

private:
	const std::string &source;
	size_t position = 0;

	[[noreturn]] void fail(const char *reason) const
	{
		throw love::Exception("Invalid glTF JSON at byte %d: %s.", (int) position, reason);
	}

	void space()
	{
		while (position < source.size() && (source[position] == ' ' || source[position] == '\t'
			|| source[position] == '\r' || source[position] == '\n'))
			position++;
	}

	bool take(char c)
	{
		space();
		if (position < source.size() && source[position] == c)
		{
			position++;
			return true;
		}
		return false;
	}

	std::string readString()
	{
		if (!take('"'))
			fail("expected string");
		std::string result;
		while (position < source.size())
		{
			char c = source[position++];
			if (c == '"')
				return result;
			if (c != '\\')
			{
				result.push_back(c);
				continue;
			}
			if (position == source.size())
				fail("unfinished escape");
			char escaped = source[position++];
			switch (escaped)
			{
			case '"': case '\\': case '/': result.push_back(escaped); break;
			case 'b': result.push_back('\b'); break;
			case 'f': result.push_back('\f'); break;
			case 'n': result.push_back('\n'); break;
			case 'r': result.push_back('\r'); break;
			case 't': result.push_back('\t'); break;
			case 'u':
				if (position + 4 > source.size())
					fail("unfinished unicode escape");
				// glTF identifiers are UTF-8. Preserve basic ASCII escapes;
				// non-ASCII escaped names are emitted as replacement UTF-8.
				{
					unsigned code = 0;
					for (int i = 0; i < 4; i++)
					{
						char h = source[position++];
						code = code * 16 + (h >= '0' && h <= '9' ? h - '0'
							: h >= 'a' && h <= 'f' ? h - 'a' + 10
							: h >= 'A' && h <= 'F' ? h - 'A' + 10 : 0);
					}
					if (code < 0x80)
						result.push_back((char) code);
					else
					{
						result.push_back((char) (0xC0 | (code >> 6)));
						result.push_back((char) (0x80 | (code & 0x3F)));
					}
				}
				break;
			default: fail("invalid escape");
			}
		}
		fail("unterminated string");
	}

	Json readValue()
	{
		space();
		if (position >= source.size())
			fail("expected value");
		char c = source[position];
		if (c == '"')
		{
			Json value;
			value.kind = Json::STRING;
			value.string = readString();
			return value;
		}
		if (c == '{')
		{
			Json value;
			value.kind = Json::OBJECT;
			position++;
			if (take('}')) return value;
			do
			{
				std::string key = readString();
				if (!take(':')) fail("expected ':'");
				value.object[key] = readValue();
				if (take('}')) return value;
				if (!take(',')) fail("expected ','");
			} while (true);
		}
		if (c == '[')
		{
			Json value;
			value.kind = Json::ARRAY;
			position++;
			if (take(']')) return value;
			do
			{
				value.array.push_back(readValue());
				if (take(']')) return value;
				if (!take(',')) fail("expected ','");
			} while (true);
		}
		if (source.compare(position, 4, "true") == 0)
		{
			position += 4;
			Json value; value.kind = Json::BOOLEAN; value.boolean = true; return value;
		}
		if (source.compare(position, 5, "false") == 0)
		{
			position += 5;
			Json value; value.kind = Json::BOOLEAN; return value;
		}
		if (source.compare(position, 4, "null") == 0)
		{
			position += 4;
			return Json();
		}
		char *end = nullptr;
		double number = strtod(source.c_str() + position, &end);
		if (end == source.c_str() + position)
			fail("invalid value");
		position = (size_t) (end - source.c_str());
		Json value; value.kind = Json::NUMBER; value.number = number; return value;
	}
};

static std::string directoryOf(const std::string &path)
{
	size_t slash = path.find_last_of('/');
	return slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
}

static std::string resolveRelativePath(const std::string &baseFile, const std::string &relative)
{
	if (relative.empty() || relative[0] == '/' || relative.find(':') != std::string::npos)
		throw love::Exception("glTF resource URI '%s' must be a relative filesystem path.", relative.c_str());

	std::vector<std::string> components;
	std::string combined = directoryOf(baseFile) + relative;
	size_t start = 0;
	while (start <= combined.size())
	{
		size_t end = combined.find('/', start);
		std::string component = combined.substr(start,
			end == std::string::npos ? std::string::npos : end - start);
		if (!component.empty() && component != ".")
		{
			if (component == "..")
			{
				if (components.empty())
					throw love::Exception("glTF resource URI '%s' escapes the filesystem root.", relative.c_str());
				components.pop_back();
			}
			else
				components.push_back(component);
		}
		if (end == std::string::npos)
			break;
		start = end + 1;
	}

	std::string path;
	for (const std::string &component : components)
	{
		if (!path.empty()) path += '/';
		path += component;
	}
	return path;
}

static std::vector<uint8> decodeBase64(const std::string &encoded)
{
	static const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::vector<uint8> output;
	unsigned accumulator = 0;
	int bits = 0;
	for (char c : encoded)
	{
		if (c == '=') break;
		size_t value = alphabet.find(c);
		if (value == std::string::npos)
			continue;
		accumulator = (accumulator << 6) | (unsigned) value;
		bits += 6;
		if (bits >= 8)
		{
			bits -= 8;
			output.push_back((uint8) ((accumulator >> bits) & 0xFF));
		}
	}
	return output;
}

static std::string decodeURIPath(const std::string &uri)
{
	std::string decoded;
	for (size_t i = 0; i < uri.size(); i++)
	{
		if (uri[i] != '%')
		{
			decoded.push_back(uri[i]);
			continue;
		}
		if (i + 2 >= uri.size())
			throw love::Exception("Malformed percent escape in glTF URI.");
		auto hex = [](char c) -> int
		{
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return -1;
		};
		int high = hex(uri[i + 1]), low = hex(uri[i + 2]);
		if (high < 0 || low < 0)
			throw love::Exception("Malformed percent escape in glTF URI.");
		decoded.push_back((char) ((high << 4) | low));
		i += 2;
	}
	return decoded;
}

static std::vector<uint8> uriData(const std::string &uri, const std::string &basePath,
	const GLTFLoader::ReadFile &readFile)
{
	if (uri.compare(0, 5, "data:") == 0)
	{
		size_t comma = uri.find(',');
		if (comma == std::string::npos || uri.substr(0, comma).find(";base64") == std::string::npos)
			throw love::Exception("glTF data URI must use base64 encoding.");
		return decodeBase64(uri.substr(comma + 1));
	}
	if (uri.compare(0, 2, "//") == 0)
		throw love::Exception("External glTF resource URI '%s' must be a relative filesystem path.", uri.c_str());
	return readFile(resolveRelativePath(basePath, decodeURIPath(uri)));
}

static uint32 readU32(const uint8 *p)
{
	return (uint32) p[0] | ((uint32) p[1] << 8) | ((uint32) p[2] << 16) | ((uint32) p[3] << 24);
}

static float component(const uint8 *data, int type, bool normalized)
{
	switch (type)
	{
	case 5120:
		{ int8 v; memcpy(&v, data, 1); return normalized ? std::max(-1.0f, v / 127.0f) : (float) v; }
	case 5121:
		{ uint8 v = data[0]; return normalized ? v / 255.0f : (float) v; }
	case 5122:
		{ int16 v; memcpy(&v, data, 2); return normalized ? std::max(-1.0f, v / 32767.0f) : (float) v; }
	case 5123:
		{ uint16 v; memcpy(&v, data, 2); return normalized ? v / 65535.0f : (float) v; }
	case 5125:
		{ uint32 v; memcpy(&v, data, 4); return normalized ? v / 4294967295.0f : (float) v; }
	case 5126:
		{ float v; memcpy(&v, data, 4); return v; }
	default:
		throw love::Exception("glTF accessor uses unsupported componentType %d.", type);
	}
}

static int componentSize(int type)
{
	switch (type)
	{
	case 5120: case 5121: return 1;
	case 5122: case 5123: return 2;
	case 5125: case 5126: return 4;
	default: return 0;
	}
}

static int typeComponents(const std::string &type)
{
	if (type == "SCALAR") return 1;
	if (type == "VEC2") return 2;
	if (type == "VEC3") return 3;
	if (type == "VEC4") return 4;
	if (type == "MAT2") return 4;
	if (type == "MAT3") return 9;
	if (type == "MAT4") return 16;
	return 0;
}

class Document
{
public:
	Json root;
	std::vector<std::vector<uint8>> buffers;
	std::vector<std::vector<uint8>> images;
	std::vector<love::graphics::SamplerState> samplers;
	std::vector<std::pair<int, int>> textureSources;
	std::map<std::tuple<int, int, bool>, StrongRef<love::graphics::Texture>> textureCache;
	love::graphics::Graphics *graphics = nullptr;
	GLTFLoader::ReadFile reader;
	GLTFLoader::LoadTexture textureLoader;
	std::string path;

	std::vector<float> accessor(int index, int expectedComponents = 0)
	{
		const Json &a = root.get("accessors").at((size_t) index);
		if (a.kind != Json::OBJECT)
			throw love::Exception("glTF references invalid accessor %d.", index);
		if (a.get("sparse").kind != Json::NIL)
			throw love::Exception("Sparse glTF accessors are not supported (accessor %d).", index);
		int componentType = a.get("componentType").integer();
		int bytesPerComponent = componentSize(componentType);
		int count = a.get("count").integer();
		int components = typeComponents(a.get("type").string);
		if (bytesPerComponent == 0 || count < 0 || components == 0
			|| (expectedComponents != 0 && components != expectedComponents))
			throw love::Exception("glTF accessor %d has an invalid or unsupported format.", index);

		std::vector<float> values((size_t) count * components, 0.0f);
		if (a.get("bufferView").kind == Json::NIL)
			return values;
		int viewIndex = a.get("bufferView").integer(-1);
		if (viewIndex < 0)
			throw love::Exception("glTF accessor %d references an invalid bufferView.", index);
		const Json &view = root.get("bufferViews").at((size_t) viewIndex);
		int bufferIndex = view.get("buffer").integer(-1);
		if (bufferIndex < 0 || (size_t) bufferIndex >= buffers.size())
			throw love::Exception("glTF accessor %d references an invalid buffer.", index);
		const std::vector<uint8> &buffer = buffers[bufferIndex];
		int viewOffset = view.get("byteOffset").integer();
		int viewBytes = view.get("byteLength").integer(-1);
		int accessorOffset = a.get("byteOffset").integer();
		if (viewOffset < 0 || viewBytes < 0 || accessorOffset < 0)
			throw love::Exception("glTF accessor %d has a negative byte range.", index);
		size_t viewStart = (size_t) viewOffset;
		size_t viewLength = (size_t) viewBytes;
		if (viewStart > buffer.size() || viewLength > buffer.size() - viewStart)
			throw love::Exception("glTF bufferView exceeds its buffer.");
		size_t start = viewStart + (size_t) a.get("byteOffset").integer();
		size_t viewEnd = viewStart + viewLength;
		size_t elementSize = (size_t) bytesPerComponent * components;
		size_t stride = (size_t) view.get("byteStride").integer((int) elementSize);
		if (stride < elementSize || start > viewEnd || elementSize > viewEnd - start
			|| (count > 1 && (size_t) (count - 1) > (viewEnd - start - elementSize) / stride))
			throw love::Exception("glTF accessor %d exceeds its buffer.", index);
		for (int i = 0; i < count; i++)
		{
			size_t at = start + (size_t) i * stride;
			for (int c = 0; c < components; c++)
				values[(size_t) i * components + c] = component(buffer.data() + at + (size_t) c * bytesPerComponent,
					componentType, a.get("normalized").boolean);
		}
		return values;
	}

	void loadBuffers(const std::vector<uint8> &glbBin)
	{
		const Json &array = root.get("buffers");
		buffers.reserve(array.array.size());
		for (size_t i = 0; i < array.array.size(); i++)
		{
			const Json &item = array.at(i);
			std::vector<uint8> bytes;
			if (item.get("uri").kind == Json::STRING)
				bytes = uriData(item.get("uri").string, path, reader);
			else if (i == 0 && !glbBin.empty())
				bytes = glbBin;
			else
				throw love::Exception("glTF buffer %d has no URI or GLB binary chunk.", (int) i);
			if (bytes.size() < (size_t) item.get("byteLength").integer())
				throw love::Exception("glTF buffer %d is shorter than byteLength.", (int) i);
			buffers.push_back(std::move(bytes));
		}
	}

	void loadTextures()
	{
		const Json &imageArray = root.get("images");
		for (const Json &image : imageArray.array)
		{
			if (image.get("uri").kind == Json::STRING)
				images.push_back(uriData(image.get("uri").string, path, reader));
			else if (image.get("bufferView").kind != Json::NIL)
			{
				const Json &view = root.get("bufferViews").at((size_t) image.get("bufferView").integer(-1));
				int bi = view.get("buffer").integer(-1);
				size_t start = (size_t) view.get("byteOffset").integer();
				size_t length = (size_t) view.get("byteLength").integer();
				if (bi < 0 || (size_t) bi >= buffers.size() || start > buffers[bi].size()
					|| length > buffers[bi].size() - start)
					throw love::Exception("glTF embedded image has an invalid bufferView.");
				images.emplace_back(buffers[bi].begin() + start, buffers[bi].begin() + start + length);
			}
			else
				throw love::Exception("glTF image has neither URI nor bufferView.");
		}

		const Json &samplerArray = root.get("samplers");
		for (const Json &input : samplerArray.array)
		{
			love::graphics::SamplerState sampler;
			sampler.wrapU = sampler.wrapV = love::graphics::SamplerState::WRAP_REPEAT;
			sampler.minFilter = sampler.magFilter = love::graphics::SamplerState::FILTER_LINEAR;
			sampler.wrapU = wrapMode(input.get("wrapS").integer(10497));
			sampler.wrapV = wrapMode(input.get("wrapT").integer(10497));
			int mag = input.get("magFilter").integer(9729);
			if (mag != 9728 && mag != 9729)
				throw love::Exception("glTF sampler uses unsupported magnification filter %d.", mag);
			sampler.magFilter = mag == 9728 ? love::graphics::SamplerState::FILTER_NEAREST : love::graphics::SamplerState::FILTER_LINEAR;
			int min = input.get("minFilter").integer(9987);
			if (min != 9728 && min != 9729 && (min < 9984 || min > 9987))
				throw love::Exception("glTF sampler uses unsupported minification filter %d.", min);
			sampler.minFilter = (min == 9728 || min == 9984 || min == 9986)
				? love::graphics::SamplerState::FILTER_NEAREST : love::graphics::SamplerState::FILTER_LINEAR;
			sampler.mipmapFilter = min < 9984 ? love::graphics::SamplerState::MIPMAP_FILTER_NONE
				: ((min == 9984 || min == 9985) ? love::graphics::SamplerState::MIPMAP_FILTER_NEAREST
					: love::graphics::SamplerState::MIPMAP_FILTER_LINEAR);
			samplers.push_back(sampler);
		}

		for (const Json &texture : root.get("textures").array)
		{
			int source = texture.get("source").integer(-1);
			int samplerIndex = texture.get("sampler").integer(-1);
			if (source < 0 || (size_t) source >= images.size())
				throw love::Exception("glTF texture references an invalid image.");
			if (samplerIndex < -1)
				throw love::Exception("glTF texture references an invalid sampler.");
			if (samplerIndex >= 0)
				if ((size_t) samplerIndex >= samplers.size())
					throw love::Exception("glTF texture references an invalid sampler.");
			textureSources.emplace_back(source, samplerIndex);
		}
	}

	love::graphics::SamplerState::WrapMode wrapMode(int value)
	{
		switch (value)
		{
		case 33071: return love::graphics::SamplerState::WRAP_CLAMP;
		case 33648: return love::graphics::SamplerState::WRAP_MIRRORED_REPEAT;
		case 10497: return love::graphics::SamplerState::WRAP_REPEAT;
		default: throw love::Exception("glTF sampler uses unsupported wrap mode %d.", value);
		}
	}

	Material3D *material(int index)
	{
		Material3D *result = new Material3D();
		if (index < 0)
			return result;
		const Json &m = root.get("materials").at((size_t) index);
		if (m.kind != Json::OBJECT)
		{
			result->release();
			throw love::Exception("glTF references invalid material %d.", index);
		}
		try
		{
			const Json &pbr = m.get("pbrMetallicRoughness");
			const Json &factor = pbr.get("baseColorFactor");
			auto toSRGB = [](double linear)
			{
				return (float) (linear <= 0.0031308 ? 12.92 * linear
					: 1.055 * pow(linear, 1.0 / 2.4) - 0.055);
			};
			result->setColor(toSRGB(factor.at(0).num(1)), toSRGB(factor.at(1).num(1)),
				toSRGB(factor.at(2).num(1)), (float) factor.at(3).num(1));
			bindTexture(pbr.get("baseColorTexture"), result, 0);
			result->setMetallic((float) pbr.get("metallicFactor").num(1.0));
			result->setRoughness(std::max(0.04f, (float) pbr.get("roughnessFactor").num(1.0)));
			result->setPBR(true);
			bindTexture(pbr.get("metallicRoughnessTexture"), result, 4);
			const Json &occlusion = m.get("occlusionTexture");
			bindTexture(occlusion, result, 5);
			result->setOcclusionStrength((float) occlusion.get("strength").num(1.0));
			bindTexture(m.get("normalTexture"), result, 1);
			bindTexture(m.get("emissiveTexture"), result, 3);
			const Json &emissive = m.get("emissiveFactor");
			result->setEmissiveColor(Vector3(toSRGB(emissive.at(0).num()), toSRGB(emissive.at(1).num()),
				toSRGB(emissive.at(2).num())));
			std::string alpha = m.get("alphaMode").kind == Json::STRING ? m.get("alphaMode").string : "OPAQUE";
			if (alpha == "BLEND") result->setAlphaMode(Material3D::ALPHA_BLEND);
			else if (alpha == "MASK") result->setAlphaMode(Material3D::ALPHA_MASK);
			else if (alpha == "OPAQUE") result->setAlphaMode(Material3D::ALPHA_OPAQUE);
			else
				throw love::Exception("glTF material uses unsupported alphaMode '%s'.", alpha.c_str());
			result->setAlphaCutoff((float) m.get("alphaCutoff").num(0.5));
		}
		catch (...)
		{
			result->release();
			throw;
		}
		return result;
	}

	void bindTexture(const Json &info, Material3D *material, int slot)
	{
		if (info.kind != Json::OBJECT)
			return;
		if (info.get("texCoord").integer() != 0)
			throw love::Exception("glTF texture coordinates other than TEXCOORD_0 are not supported.");
		int index = info.get("index").integer(-1);
		if (index < 0 || (size_t) index >= textureSources.size())
			throw love::Exception("glTF material references an invalid texture.");
		std::pair<int, int> source = textureSources[index];
		bool linear = slot == 1 || slot == 4 || slot == 5;
		std::tuple<int, int, bool> key(source.first, source.second, linear);
		auto it = textureCache.find(key);
		if (it == textureCache.end())
		{
			love::graphics::SamplerState sampler;
			sampler.wrapU = sampler.wrapV = love::graphics::SamplerState::WRAP_REPEAT;
			sampler.minFilter = sampler.magFilter = love::graphics::SamplerState::FILTER_LINEAR;
			if (source.second >= 0)
				sampler = samplers[source.second];
			love::graphics::Texture *loaded = textureLoader(images[source.first], sampler, linear);
			try
			{
				it = textureCache.emplace(key, StrongRef<love::graphics::Texture>(loaded)).first;
			}
			catch (...)
			{
				loaded->release();
				throw;
			}
			loaded->release();
		}
		love::graphics::Texture *texture = it->second.get();
		if (slot == 0) material->setDiffuseTexture(texture);
		else if (slot == 1) material->setNormalTexture(texture);
		else if (slot == 3) material->setEmissiveTexture(texture);
		else if (slot == 4) material->setMetallicRoughnessTexture(texture);
		else if (slot == 5) material->setOcclusionTexture(texture);
	}
};

static void checkExtensions(const Json &root)
{
	const Json &used = root.get("extensionsUsed");
	const Json &required = root.get("extensionsRequired");
	for (const Json *array : { &used, &required })
		for (const Json &extension : array->array)
			throw love::Exception("Unsupported glTF extension '%s'.", extension.string.c_str());
}

static void generateNormals(std::vector<Vertex3D> &vertices, const std::vector<uint32> &indices)
{
	std::vector<Vector3> normals(vertices.size(), Vector3());
	for (size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		uint32 a = indices[i], b = indices[i + 1], c = indices[i + 2];
		Vector3 p(vertices[a].position[0], vertices[a].position[1], vertices[a].position[2]);
		Vector3 q(vertices[b].position[0], vertices[b].position[1], vertices[b].position[2]);
		Vector3 r(vertices[c].position[0], vertices[c].position[1], vertices[c].position[2]);
		Vector3 n = (q - p).cross(r - p);
		normals[a] += n; normals[b] += n; normals[c] += n;
	}
	for (size_t i = 0; i < vertices.size(); i++)
	{
		float length = normals[i].length();
		if (length > 1e-8f)
			normals[i] = normals[i] / length;
		memcpy(vertices[i].normal, &normals[i], sizeof(float) * 3);
	}
}

} // anonymous namespace

Model3D *GLTFLoader::load(const std::string &path, love::graphics::Graphics *graphics,
	const ReadFile &readFile, const LoadTexture &loadTexture)
{
	std::vector<uint8> file = readFile(path);
	std::string jsonText;
	std::vector<uint8> bin;
	if (file.size() >= 12 && readU32(file.data()) == 0x46546C67)
	{
		if (readU32(file.data() + 4) != 2 || readU32(file.data() + 8) != file.size())
			throw love::Exception("Invalid GLB header or unsupported GLB version in '%s'.", path.c_str());
		size_t offset = 12;
		while (offset + 8 <= file.size())
		{
			uint32 length = readU32(file.data() + offset);
			uint32 type = readU32(file.data() + offset + 4);
			offset += 8;
			if (length > file.size() - offset)
				throw love::Exception("GLB chunk exceeds file bounds in '%s'.", path.c_str());
			if (type == 0x4E4F534A && jsonText.empty())
				jsonText.assign((const char *) file.data() + offset, length);
			else if (type == 0x004E4942 && bin.empty())
				bin.assign(file.begin() + offset, file.begin() + offset + length);
			offset += length;
		}
	}
	else
		jsonText.assign((const char *) file.data(), file.size());

	Document doc;
	doc.root = JsonParser(jsonText).parse();
	const Json &assetVersion = doc.root.get("asset").get("version");
	if (assetVersion.kind != Json::STRING || assetVersion.string != "2.0")
		throw love::Exception("'%s' is not a glTF 2.0 asset.", path.c_str());
	checkExtensions(doc.root);
	doc.graphics = graphics;
	doc.reader = readFile;
	doc.textureLoader = loadTexture;
	doc.path = path;
	doc.loadBuffers(bin);
	doc.loadTextures();

	Model3D *model = new Model3D();
	std::vector<StrongRef<Mesh3D>> meshes;
	std::vector<StrongRef<Material3D>> materials;
	const Json &meshArray = doc.root.get("meshes");
	try
	{
		for (const Json &mesh : meshArray.array)
		{
			for (const Json &primitive : mesh.get("primitives").array)
			{
				if (primitive.get("mode").integer(4) != 4)
					throw love::Exception("glTF primitive mode other than TRIANGLES is not supported.");
				const Json &attributes = primitive.get("attributes");
				if (attributes.get("POSITION").kind == Json::NIL)
					throw love::Exception("glTF primitive is missing POSITION.");
				std::vector<float> positions = doc.accessor(attributes.get("POSITION").integer(), 3);
				size_t count = positions.size() / 3;
				std::vector<float> normals, uvs, tangents, colors;
				if (attributes.get("NORMAL").kind != Json::NIL)
					normals = doc.accessor(attributes.get("NORMAL").integer(), 3);
				if (attributes.get("TEXCOORD_0").kind != Json::NIL)
					uvs = doc.accessor(attributes.get("TEXCOORD_0").integer(), 2);
				if (attributes.get("TANGENT").kind != Json::NIL)
					tangents = doc.accessor(attributes.get("TANGENT").integer(), 4);
				int colorComponents = 0;
				if (attributes.get("COLOR_0").kind != Json::NIL)
				{
					int colorAccessor = attributes.get("COLOR_0").integer();
					const Json &colorType = doc.root.get("accessors").at((size_t) colorAccessor).get("type");
					colorComponents = colorType.string == "VEC3" ? 3 : colorType.string == "VEC4" ? 4 : 0;
					if (colorComponents == 0)
						throw love::Exception("glTF COLOR_0 must be VEC3 or VEC4.");
					colors = doc.accessor(colorAccessor, colorComponents);
				}
				if ((!normals.empty() && normals.size() != count * 3)
					|| (!uvs.empty() && uvs.size() != count * 2)
					|| (!tangents.empty() && tangents.size() != count * 4)
					|| (!colors.empty() && colors.size() != count * colorComponents))
					throw love::Exception("glTF vertex attribute counts do not match POSITION.");
				std::vector<Vertex3D> vertices(count);
				for (size_t i = 0; i < count; i++)
				{
					Vertex3D &v = vertices[i];
					memcpy(v.position, &positions[i * 3], sizeof(float) * 3);
					if (!normals.empty()) memcpy(v.normal, &normals[i * 3], sizeof(float) * 3);
					else memset(v.normal, 0, sizeof(v.normal));
					if (!uvs.empty()) { v.texcoord[0] = uvs[i * 2]; v.texcoord[1] = 1.0f - uvs[i * 2 + 1]; }
					else v.texcoord[0] = v.texcoord[1] = 0.0f;
					for (float &color : v.color) color = 1.0f;
					if (!colors.empty())
					{
						for (int c = 0; c < colorComponents; c++)
							v.color[c] = colors[i * colorComponents + c];
					}
					if (!tangents.empty())
					{
						v.tangent[0] = tangents[i * 4];
						v.tangent[1] = tangents[i * 4 + 1];
						v.tangent[2] = tangents[i * 4 + 2];
						v.tangent[3] = tangents[i * 4 + 3] * (uvs.empty() ? 1.0f : -1.0f);
					}
				}
				std::vector<uint32> indices;
				if (primitive.get("indices").kind != Json::NIL)
				{
					int accessorIndex = primitive.get("indices").integer();
					const Json &indexAccessor = doc.root.get("accessors").at((size_t) accessorIndex);
					int indexType = indexAccessor.get("componentType").integer();
					if ((indexType != 5121 && indexType != 5123 && indexType != 5125)
						|| indexAccessor.get("normalized").boolean)
						throw love::Exception("glTF indices must use unsigned byte, short, or int components.");
					std::vector<float> raw = doc.accessor(accessorIndex, 1);
					indices.reserve(raw.size());
					for (float value : raw)
					{
						if (value < 0 || value >= count || floor(value) != value)
							throw love::Exception("glTF index accessor contains an invalid vertex index.");
						indices.push_back((uint32) value);
					}
				}
				else
				{
					indices.resize(count);
					for (size_t i = 0; i < count; i++) indices[i] = (uint32) i;
				}
				if (normals.empty()) generateNormals(vertices, indices);
				Mesh3D *partMesh = new Mesh3D(graphics, vertices, indices);
				meshes.emplace_back(partMesh);
				partMesh->release();
				int materialIndex = primitive.get("material").integer(-1);
				Material3D *partMaterial = doc.material(materialIndex);
				materials.emplace_back(partMaterial);
				partMaterial->release();
			}
		}

		std::vector<bool> visited(doc.root.get("nodes").array.size(), false);
		std::function<void(int, const Matrix4 &)> visit = [&](int nodeIndex, const Matrix4 &parent)
		{
			if (nodeIndex < 0 || (size_t) nodeIndex >= visited.size())
				throw love::Exception("glTF node references an invalid child.");
			if (visited[nodeIndex])
				throw love::Exception("glTF node hierarchy contains a cycle or multiply-parented node.");
			visited[nodeIndex] = true;
			const Json &node = doc.root.get("nodes").at((size_t) nodeIndex);
			Matrix4 local;
			const Json &matrix = node.get("matrix");
			if (matrix.kind == Json::ARRAY && matrix.array.size() == 16)
				for (int i = 0; i < 16; i++) local.m[i] = (float) matrix.at(i).num();
			else
			{
				Vector3 translation((float) node.get("translation").at(0).num(),
					(float) node.get("translation").at(1).num(), (float) node.get("translation").at(2).num());
				Vector3 scale((float) node.get("scale").at(0).num(1),
					(float) node.get("scale").at(1).num(1), (float) node.get("scale").at(2).num(1));
				const Json &q = node.get("rotation");
				Quaternion rotation((float) q.at(0).num(), (float) q.at(1).num(),
					(float) q.at(2).num(), (float) q.at(3).num(1));
				local.setTransformQ(translation, rotation, scale);
			}
			Matrix4 world = parent * local;
			if (node.get("mesh").kind != Json::NIL)
			{
				int meshIndex = node.get("mesh").integer(-1);
				if (meshIndex < 0 || (size_t) meshIndex >= meshArray.array.size())
					throw love::Exception("glTF node references an invalid mesh.");
				size_t first = 0;
				for (int i = 0; i < meshIndex; i++)
					first += meshArray.at(i).get("primitives").array.size();
				size_t count = meshArray.at(meshIndex).get("primitives").array.size();
				for (size_t i = first; i < first + count; i++)
					model->addPart(meshes[i].get(), materials[i].get(), world);
			}
			for (const Json &child : node.get("children").array)
				visit(child.integer(-1), world);
		};

		const Json &scenes = doc.root.get("scenes");
		if (scenes.kind == Json::ARRAY && !scenes.array.empty())
		{
			int sceneIndex = doc.root.get("scene").integer(0);
			for (const Json &node : scenes.at((size_t) sceneIndex).get("nodes").array)
				visit(node.integer(-1), Matrix4::identity());
		}
		else
		{
			std::vector<bool> child(visited.size(), false);
			for (const Json &node : doc.root.get("nodes").array)
				for (const Json &childNode : node.get("children").array)
					if (childNode.integer(-1) >= 0 && (size_t) childNode.integer() < child.size())
						child[childNode.integer()] = true;
			for (size_t i = 0; i < child.size(); i++)
				if (!child[i]) visit((int) i, Matrix4::identity());
		}
		if (model->getPartCount() == 0)
			throw love::Exception("glTF file contains no drawable mesh in its selected scene.");
	}
	catch (...)
	{
		model->release();
		throw;
	}
	return model;
}

} // graphics3d
} // love
