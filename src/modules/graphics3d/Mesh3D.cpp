/**
 * LOVING3D - 3D mesh implementation.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org). The underlying engine
 * remains freely distributable under the terms of its original zlib license
 * (see license.txt).
 **/

#include "Mesh3D.h"
#include "Material3D.h"

#include "common/Exception.h"

#include <cstring>
#include <algorithm>
#include <cmath>
#include <limits>

namespace love
{
namespace graphics3d
{

/**
 * The interleaved 3D vertex attribute declarations. Declaration binding
 * locations must match the layout(location = N) declarations in the
 * module's shaders (see Graphics3D.cpp).
 **/
static const std::vector<love::graphics::Buffer::DataDeclaration> group3DVertexDeclarations = {
	{ "Position", love::graphics::DATAFORMAT_FLOAT_VEC3, 0, 0 },
	{ "Normal", love::graphics::DATAFORMAT_FLOAT_VEC3, 0, 1 },
	{ "TexCoord", love::graphics::DATAFORMAT_FLOAT_VEC2, 0, 2 },
	{ "Color", love::graphics::DATAFORMAT_FLOAT_VEC4, 0, 3 },
	{ "Tangent", love::graphics::DATAFORMAT_FLOAT_VEC4, 0, 4 },
};

static bool isFiniteVertex(const Vertex3D &vertex)
{
	const float *attributes[] = {
		vertex.position,
		vertex.normal,
		vertex.texcoord,
		vertex.color,
		vertex.tangent,
	};
	const size_t attributeSizes[] = { 3, 3, 2, 4, 4 };

	for (size_t attribute = 0; attribute < 5; attribute++)
	{
		for (size_t i = 0; i < attributeSizes[attribute]; i++)
		{
			if (!std::isfinite(attributes[attribute][i]))
				return false;
		}
	}

	return true;
}

static void generateTangents(std::vector<Vertex3D> &vertices, const std::vector<uint32> &indices)
{
	std::vector<Vector3> tangent(vertices.size(), Vector3());
	std::vector<Vector3> bitangent(vertices.size(), Vector3());
	for (size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		uint32 ia = indices[i], ib = indices[i + 1], ic = indices[i + 2];
		Vertex3D &a = vertices[ia];
		Vertex3D &b = vertices[ib];
		Vertex3D &c = vertices[ic];
		Vector3 p0(a.position[0], a.position[1], a.position[2]);
		Vector3 p1(b.position[0], b.position[1], b.position[2]);
		Vector3 p2(c.position[0], c.position[1], c.position[2]);
		Vector3 e1 = p1 - p0, e2 = p2 - p0;
		float du1 = b.texcoord[0] - a.texcoord[0], dv1 = b.texcoord[1] - a.texcoord[1];
		float du2 = c.texcoord[0] - a.texcoord[0], dv2 = c.texcoord[1] - a.texcoord[1];
		float determinant = du1 * dv2 - du2 * dv1;
		if (fabsf(determinant) < 1e-8f)
			continue;
		float inverse = 1.0f / determinant;
		Vector3 t = (e1 * dv2 - e2 * dv1) * inverse;
		Vector3 bt = (e2 * du1 - e1 * du2) * inverse;
		tangent[ia] += t; tangent[ib] += t; tangent[ic] += t;
		bitangent[ia] += bt; bitangent[ib] += bt; bitangent[ic] += bt;
	}

	for (size_t i = 0; i < vertices.size(); i++)
	{
		if (vertices[i].tangent[3] != 0.0f)
			continue;
		Vertex3D &v = vertices[i];
		Vector3 n(v.normal[0], v.normal[1], v.normal[2]);
		n.normalize();
		Vector3 t = tangent[i] - n * n.dot(tangent[i]);
		if (t.lengthsq() < 1e-12f)
			t = (fabsf(n.y) < 0.9f ? Vector3(0, 1, 0) : Vector3(1, 0, 0)).cross(n);
		t.normalize();
		float handedness = n.cross(t).dot(bitangent[i]) < 0.0f ? -1.0f : 1.0f;
		v.tangent[0] = t.x; v.tangent[1] = t.y; v.tangent[2] = t.z; v.tangent[3] = handedness;
	}
}

love::Type Mesh3D::type("Mesh3D", &Object::type);

Mesh3D::Mesh3D(love::graphics::Graphics *gfx, const std::vector<Vertex3D> &vertices_, const std::vector<uint32> &indices_, love::graphics::BufferDataUsage usage)
{
	vertexCount = vertices_.size();
	indexCount = indices_.size();

	if (vertexCount == 0)
		throw love::Exception("A Mesh3D needs at least one vertex.");

	if (vertexCount > (size_t) 0xFFFFFFFE)
		throw love::Exception("Mesh3D supports up to 4294967294 vertices per mesh.");

	if (vertexCount > std::numeric_limits<size_t>::max() / sizeof(Vertex3D))
		throw love::Exception("Mesh3D vertex data is too large.");

	if (indexCount % 3 != 0)
		throw love::Exception("Mesh3D index count must be a multiple of 3.");

	std::vector<Vertex3D> generatedVertices = vertices_;
	generateTangents(generatedVertices, indices_);
	for (const Vertex3D &vertex : generatedVertices)
	{
		if (!isFiniteVertex(vertex))
			throw love::Exception("Mesh3D vertex data must contain only finite values.");
	}

	for (uint32 index : indices_)
	{
		if (index >= vertexCount)
			throw love::Exception("Mesh3D index is outside the vertex range.");
	}

	// CPU-side copy for setIndexData / upload() updates.
	vertexData = std::move(generatedVertices);
	indexData = indices_;

	// 32-bit indices keep the CPU cache and the GPU format identical.
	indexDataType = love::graphics::INDEX_UINT32;

	love::graphics::Buffer::Settings settings(love::graphics::BUFFERUSAGEFLAG_VERTEX, usage);
	settings.debugName = "graphics3d vertex mesh";

	size_t datasize = vertexCount * sizeof(Vertex3D);

	vertexBuffer.set(gfx->newBuffer(settings, group3DVertexDeclarations, vertexData.data(), datasize, 0), Acquire::NORETAIN);

	if (vertexCount != vertexBuffer.get()->getArrayLength())
		throw love::Exception("Vertex buffer size mismatch when creating Mesh3D.");

	stride = vertexBuffer.get()->getArrayStride();
	if (stride != sizeof(Vertex3D))
		throw love::Exception("Unexpected vertex buffer stride when creating Mesh3D.");

	if (indexCount > 0)
	{
		love::graphics::Buffer::Settings idxSettings(love::graphics::BUFFERUSAGEFLAG_INDEX, usage);
		idxSettings.debugName = "graphics3d index mesh";

		love::graphics::DataFormat iformat = love::graphics::getIndexDataFormat(indexDataType);
		size_t indexElementSize = love::graphics::getIndexDataSize(indexDataType);
		if (indexCount > std::numeric_limits<size_t>::max() / indexElementSize)
			throw love::Exception("Mesh3D index data is too large.");
		size_t idxbytesize = indexCount * indexElementSize;

		// The backend copies the data into the GPU buffer at creation.
		indexBuffer.set(gfx->newBuffer(idxSettings, iformat, indexData.data(), idxbytesize, 0), Acquire::NORETAIN);
	}
}

Mesh3D::~Mesh3D() = default;

void Mesh3D::setVertexData(const Vertex3D *data, size_t count, size_t startIndex)
{
	if (data == nullptr || count == 0)
		return;

	if (startIndex > vertexCount || count > vertexCount - startIndex)
		throw love::Exception("Vertex data range out of bounds.");

	for (size_t i = 0; i < count; i++)
	{
		if (!isFiniteVertex(data[i]))
			throw love::Exception("Mesh3D vertex data must contain only finite values.");
	}

	memcpy(vertexData.data() + startIndex, data, count * sizeof(Vertex3D));
	generateTangents(vertexData, indexData);
	vertexDataDirty = true;
	boundsDirty = true;
	transparencyDirty = true;
}

void Mesh3D::setIndexData(const uint32 *data, size_t count, size_t startIndex)
{
	if (data == nullptr || count == 0)
		return;

	if (startIndex > indexCount || count > indexCount - startIndex)
		throw love::Exception("Index data range out of bounds.");

	for (size_t i = 0; i < count; i++)
	{
		if (data[i] >= vertexCount)
			throw love::Exception("Mesh3D index is outside the vertex range.");
	}

	memcpy(indexData.data() + startIndex, data, count * sizeof(uint32));
	indexDataDirty = true;
}

void Mesh3D::setMaterial(Material3D *material_)
{
	if (material_ == nullptr)
		throw love::Exception("Expected a Material3D for setMaterial.");

	material.set(material_);
}

void Mesh3D::upload()
{
	if (vertexBuffer.get() != nullptr && vertexDataDirty)
	{
		love::graphics::Buffer::Mapper mapper(*vertexBuffer);
		memcpy(mapper.data, vertexData.data(), vertexCount * stride);
		vertexDataDirty = false;
	}

	if (indexBuffer.get() != nullptr && indexDataDirty)
	{
		love::graphics::Buffer::Mapper mapper(*indexBuffer);
		size_t bytesize = indexCount * love::graphics::getIndexDataSize(indexDataType);
		memcpy(mapper.data, indexData.data(), bytesize);
		indexDataDirty = false;
	}
}

bool Mesh3D::getBoundingBox(Vector3 &minv, Vector3 &maxv) const
{
	if (vertexCount == 0)
		return false;

	if (boundsDirty)
		updateBounds();

	minv = boundsMin;
	maxv = boundsMax;
	return true;
}

bool Mesh3D::hasTransparentVertex() const
{
	if (transparencyDirty)
	{
		vertexTransparency = false;
		for (const Vertex3D &vertex : vertexData)
		{
			if (vertex.color[3] < 1.0f)
			{
				vertexTransparency = true;
				break;
			}
		}
		transparencyDirty = false;
	}

	return vertexTransparency;
}

void Mesh3D::updateBounds() const
{
	boundsMin = Vector3(vertexData[0].position[0], vertexData[0].position[1], vertexData[0].position[2]);
	boundsMax = boundsMin;

	for (size_t i = 1; i < vertexCount; i++)
	{
		const float *pos = vertexData[i].position;
		boundsMin.x = std::min(boundsMin.x, pos[0]);
		boundsMin.y = std::min(boundsMin.y, pos[1]);
		boundsMin.z = std::min(boundsMin.z, pos[2]);
		boundsMax.x = std::max(boundsMax.x, pos[0]);
		boundsMax.y = std::max(boundsMax.y, pos[1]);
		boundsMax.z = std::max(boundsMax.z, pos[2]);
	}

	boundsDirty = false;
}

void Mesh3D::updateVertexAttributes(love::graphics::Graphics *gfx)
{
	if (attributesID.isValid())
		return;

	love::graphics::VertexAttributes attributes;

	// All attributes live inside the one interleaved vertex buffer.
	attributes.set(0, love::graphics::DATAFORMAT_FLOAT_VEC3, (uint16) offsetof(Vertex3D, position), 0);
	attributes.set(1, love::graphics::DATAFORMAT_FLOAT_VEC3, (uint16) offsetof(Vertex3D, normal), 0);
	attributes.set(2, love::graphics::DATAFORMAT_FLOAT_VEC2, (uint16) offsetof(Vertex3D, texcoord), 0);
	attributes.set(3, love::graphics::DATAFORMAT_FLOAT_VEC4, (uint16) offsetof(Vertex3D, color), 0);
	attributes.set(4, love::graphics::DATAFORMAT_FLOAT_VEC4, (uint16) offsetof(Vertex3D, tangent), 0);

	attributes.setBufferLayout(0, (uint16) stride, love::graphics::STEP_PER_VERTEX);

	attributesID = gfx->registerVertexAttributes(attributes);
}

void Mesh3D::drawBatched(love::graphics::Graphics *gfx, love::graphics::CullMode cullMode, love::graphics::Texture *texture)
{
	if (vertexBuffer.get() == nullptr || indexBuffer.get() == nullptr || indexCount == 0)
		return;

	upload();

	updateVertexAttributes(gfx);

	love::graphics::BufferBindings bindings;
	bindings.set(0, vertexBuffer.get(), 0);

	love::graphics::Graphics::DrawIndexedCommand cmd(attributesID, &bindings, indexBuffer.get());

	cmd.primitiveType = love::graphics::PRIMITIVE_TRIANGLES;
	cmd.indexType = indexDataType;
	cmd.indexCount = (int) indexCount;
	cmd.instanceCount = 1;
	cmd.indexBufferOffset = 0;
	cmd.cullMode = cullMode;

	// The GL backend dereferences cmd.texture unconditionally, so textureless
	// draws must fall back to the engine's default (white pixel) texture.
	cmd.texture = gfx->getTextureOrDefaultForActiveShader(texture);

	gfx->draw(cmd);
}

void Mesh3D::drawInstanced(love::graphics::Graphics *gfx, const std::vector<Matrix4> &transforms,
	love::graphics::CullMode cullMode, love::graphics::Texture *texture)
{
	if (transforms.empty() || vertexCount == 0 || indexCount == 0)
		return;

	upload();
	updateVertexAttributes(gfx);

	std::vector<float> instanceData;
	instanceData.reserve(transforms.size() * 16);
	for (const Matrix4 &transform : transforms)
		instanceData.insert(instanceData.end(), transform.m, transform.m + 16);

	const std::vector<love::graphics::Buffer::DataDeclaration> declarations = {
		{ "InstanceColumn0", love::graphics::DATAFORMAT_FLOAT_VEC4, 0, 5 },
		{ "InstanceColumn1", love::graphics::DATAFORMAT_FLOAT_VEC4, 0, 6 },
		{ "InstanceColumn2", love::graphics::DATAFORMAT_FLOAT_VEC4, 0, 7 },
		{ "InstanceColumn3", love::graphics::DATAFORMAT_FLOAT_VEC4, 0, 8 },
	};
	love::graphics::Buffer::Settings settings(love::graphics::BUFFERUSAGEFLAG_VERTEX,
		love::graphics::BUFFERDATAUSAGE_STREAM);
	settings.debugName = "graphics3d instance transforms";
	StrongRef<love::graphics::Buffer> instanceBuffer(
		gfx->newBuffer(settings, declarations, instanceData.data(), instanceData.size() * sizeof(float), transforms.size()),
		Acquire::NORETAIN);

	love::graphics::VertexAttributes attributes;
	attributes.set(0, love::graphics::DATAFORMAT_FLOAT_VEC3, (uint16) offsetof(Vertex3D, position), 0);
	attributes.set(1, love::graphics::DATAFORMAT_FLOAT_VEC3, (uint16) offsetof(Vertex3D, normal), 0);
	attributes.set(2, love::graphics::DATAFORMAT_FLOAT_VEC2, (uint16) offsetof(Vertex3D, texcoord), 0);
	attributes.set(3, love::graphics::DATAFORMAT_FLOAT_VEC4, (uint16) offsetof(Vertex3D, color), 0);
	attributes.set(4, love::graphics::DATAFORMAT_FLOAT_VEC4, (uint16) offsetof(Vertex3D, tangent), 0);
	attributes.setBufferLayout(0, (uint16) stride, love::graphics::STEP_PER_VERTEX);
	for (int column = 0; column < 4; column++)
		attributes.set(5 + column, love::graphics::DATAFORMAT_FLOAT_VEC4, (uint16) (column * sizeof(float) * 4), 1);
	attributes.setBufferLayout(1, (uint16) (sizeof(float) * 16), love::graphics::STEP_PER_INSTANCE);
	love::graphics::VertexAttributesID instancedAttributes = gfx->registerVertexAttributes(attributes);

	love::graphics::BufferBindings bindings;
	bindings.set(0, vertexBuffer.get(), 0);
	bindings.set(1, instanceBuffer.get(), 0);

	love::graphics::Graphics::DrawIndexedCommand cmd(instancedAttributes, &bindings, indexBuffer.get());
	cmd.primitiveType = love::graphics::PRIMITIVE_TRIANGLES;
	cmd.indexType = indexDataType;
	cmd.indexCount = (int) indexCount;
	cmd.instanceCount = (int) transforms.size();
	cmd.indexBufferOffset = 0;
	cmd.cullMode = cullMode;
	cmd.texture = gfx->getTextureOrDefaultForActiveShader(texture);
	gfx->draw(cmd);
}

void Mesh3D::addLOD(float distance, Mesh3D *mesh)
{
	if (!std::isfinite(distance) || distance <= 0.0f)
		throw love::Exception("LOD distance must be a positive finite value.");
	if (mesh == nullptr || mesh == this)
		throw love::Exception("LOD mesh must be a different valid Mesh3D.");

	for (auto &lod : lods)
	{
		if (lod.first == distance)
		{
			lod.second.set(mesh);
			return;
		}
	}

	lods.emplace_back(distance, StrongRef<Mesh3D>(mesh));
	std::sort(lods.begin(), lods.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
}

Mesh3D *Mesh3D::getLOD(float distance) const
{
	Mesh3D *selected = const_cast<Mesh3D *>(this);
	for (const auto &lod : lods)
	{
		if (distance < lod.first)
			break;
		selected = lod.second.get();
	}
	return selected;
}

} // graphics3d
} // love
