/**
 * LOVING3D - Wavefront OBJ/MTL importer implementation.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org). The underlying engine
 * remains freely distributable under the terms of its original zlib license
 * (see license.txt).
 **/

#include "OBJLoader.h"

#include "common/Exception.h"

#include "modules/graphics/Graphics.h"
#include "modules/graphics/Buffer.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <sstream>

namespace love
{
namespace graphics3d
{

namespace
{

/**
 * Whitespace-delimited line tokenizer for the OBJ/MTL text formats.
 * Comments (leading '#') and blank lines are skipped.
 **/
class LineReader
{
public:

	LineReader(const char *data, size_t size)
		: ptr(data)
		, end(data + size)
	{
	}

	/**
	 * Reads the next non-empty, non-comment line into `line` (-leading tag
	 * and arguments are given verbatim after comment removal). Returns
	 * false at the end of input.
	 **/
	bool nextLine(std::string &line)
	{
		line.clear();

		while (ptr < end)
		{
			line.clear();

			while (ptr < end && *ptr != '\n')
			{
				line.push_back(*ptr);
				ptr++;
			}

			if (ptr < end)
				ptr++; // Skip the newline.

			// Strip '\r' from Windows CR endings.
			while (!line.empty() && (line.back() == '\r'))
				line.pop_back();

			// Skip blank lines and comments.
			size_t firstNonSpace = line.find_first_not_of(" \t");
			if (firstNonSpace == std::string::npos || line[firstNonSpace] == '#')
				continue;

			trim(line);
			return true;
		}

		return false;
	}

private:

	static void trim(std::string &s)
	{
		size_t start = s.find_first_not_of(" \t\r\n");
		size_t endpos = s.find_last_not_of(" \t\r\n");
		s = (start == std::string::npos) ? std::string() : s.substr(start, endpos - start + 1);
	}

	const char *ptr;
	const char *end;

};

Vector3 parseVec3(const char *a, const char *b, const char *c)
{
	return Vector3((float) atof(a), (float) atof(b), (float) atof(c));
}

} // anonymous namespace

Model3D *OBJLoader::createModel(love::graphics::Graphics *gfx, love::graphics::BufferDataUsage usage,
	const std::function<Material3D *(const MaterialRecord &)> &materialBinder)
{
	std::map<std::string, Material3D *> materialByName;

	Model3D *model = new Model3D();

	try
	{
		for (const GroupRecord &group : groups)
		{
			if (group.geometry.vertices.empty())
				continue;

			StrongRef<Mesh3D> mesh(new Mesh3D(gfx, group.geometry.vertices, group.geometry.indices, usage), Acquire::NORETAIN);

			// Locate or create the material for this part.
			Material3D *partMaterial = nullptr;

			// Holds the initial reference of a module-created default material
			// until the part's strong reference takes over.
			StrongRef<Material3D> defaultMaterial;

			if (!group.materialName.empty())
			{
				auto it = materialByName.find(group.materialName);
				if (it != materialByName.end())
				{
					partMaterial = it->second;
				}
				else
				{
					// Ask the binder (Lua) for the material by record. The
					// binder keeps its own reference alive, so the returned
					// pointer is borrowed here.
					const MaterialRecord *record = nullptr;
					for (const MaterialRecord &m : materials)
					{
						if (m.name == group.materialName)
						{
							record = &m;
							break;
						}
					}

					MaterialRecord fallback;
					if (record == nullptr)
					{
						fallback.name = group.materialName;
						record = &fallback;
					}

					partMaterial = materialBinder(*record);

					if (partMaterial == nullptr)
					{
						// Default material from the record's raw values.
						defaultMaterial.set(new Material3D(), Acquire::NORETAIN);
						partMaterial = defaultMaterial.get();
						partMaterial->setColor(record->diffuse.x, record->diffuse.y, record->diffuse.z, record->opacity);
						partMaterial->setSpecularParams(record->specular.length(), record->shininess);
						partMaterial->setEmissiveColor(record->emissive);
						if (record->opacity < 1.0f)
							partMaterial->setOpacity(record->opacity);
					}

					materialByName[group.materialName] = partMaterial;
				}
			}

			// addPart retains the mesh and material; the guard releases the
			// mesh's initial reference after ownership transfers.
			model->addPart(mesh.get(), partMaterial, Matrix4::identity());
		}
	}
	catch (...)
	{
		model->release();
		throw;
	}

	if (model->getPartCount() == 0)
	{
		model->release();
		return nullptr;
	}

	return model;
}

uint32 OBJLoader::resolveVertex(int pIndex, int tIndex, int nIndex, GroupRecord &group)
{
	// OBJ indices are 1-based; negative values count backwards from the
	// end of the element list. A missing texcoord/normal (0) means none.
	int p = pIndex;
	int t = tIndex == 0 ? -1 : tIndex;
	int n = nIndex == 0 ? -1 : nIndex;

	if (p > 0)
		p--;
	else if (p < 0)
		p += (int) positions.size();

	if (t > 0)
		t--;
	else if (t < 0)
	{
		if (tIndex < 0)
			t += (int) texcoords.size();
	}

	if (n > 0)
		n--;
	else if (n < 0)
	{
		if (nIndex < 0)
			n += (int) normals.size();
	}

	if (p < 0 || p >= (int) positions.size())
		throw love::Exception("OBJ face references an out-of-range position index.");

	if (t != -1 && (t < 0 || t >= (int) texcoords.size()))
		throw love::Exception("OBJ face references an out-of-range texcoord index.");

	if (n != -1 && (n < 0 || n >= (int) normals.size()))
		throw love::Exception("OBJ face references an out-of-range normal index.");

	std::string key = std::to_string(p) + "/" + std::to_string(t) + "/" + std::to_string(n);

	auto it = vertexCache.find(key);
	if (it != vertexCache.end())
		return it->second;

	Vertex3D vert;
	vert.position[0] = positions[p].x;
	vert.position[1] = positions[p].y;
	vert.position[2] = positions[p].z;
	vert.normal[0] = 0.0f;
	vert.normal[1] = 0.0f;
	vert.normal[2] = 0.0f;
	vert.texcoord[0] = 0.0f;
	vert.texcoord[1] = 0.0f;
	vert.color[0] = 1.0f;
	vert.color[1] = 1.0f;
	vert.color[2] = 1.0f;
	vert.color[3] = 1.0f;

	if (t != -1)
	{
		// OBJ texture coordinates have the origin at the bottom left;
		// LOVE images sample from the top left, so flip V.
		vert.texcoord[0] = texcoords[t].x;
		vert.texcoord[1] = 1.0f - texcoords[t].y;
	}

	if (n != -1)
	{
		vert.normal[0] = normals[n].x;
		vert.normal[1] = normals[n].y;
		vert.normal[2] = normals[n].z;
	}

	uint32 index = (uint32) group.geometry.vertices.size();
	group.geometry.vertices.push_back(vert);
	vertexCache[key] = index;
	return index;
}

void OBJLoader::parseMTL(const char *content, size_t contentSize)
{
	LineReader reader(content, contentSize);
	std::string line;

	while (reader.nextLine(line))
	{
		char tag[32];
		char name[256];

		if (sscanf(line.c_str(), "newmtl %255s", name) == 1)
		{
			MaterialRecord m;
			m.name = name;
			materials.push_back(m);
			continue;
		}

		if (materials.empty())
			continue;

		MaterialRecord &mtl = materials.back();

		float x = 0.0f, y = 0.0f, z = 0.0f;
		char texfile[512];

		if (sscanf(line.c_str(), "Kd %f %f %f", &x, &y, &z) == 3)
			mtl.diffuse = Vector3(x, y, z);
		else if (sscanf(line.c_str(), "Ka %f %f %f", &x, &y, &z) == 3)
			mtl.ambient = Vector3(x, y, z);
		else if (sscanf(line.c_str(), "Ke %f %f %f", &x, &y, &z) == 3)
			mtl.emissive = Vector3(x, y, z);
		else if (sscanf(line.c_str(), "Ks %f %f %f", &x, &y, &z) == 3)
			mtl.specular = Vector3(x, y, z);
		else if (sscanf(line.c_str(), "Ns %f", &x) == 1)
			mtl.shininess = x;
		else if (sscanf(line.c_str(), "d %f", &x) == 1)
			mtl.opacity = x;
		else if (sscanf(line.c_str(), "Tr %f", &x) == 1)
			mtl.opacity = 1.0f - x;
		else if (sscanf(line.c_str(), "map_Kd %511s", texfile) == 1)
			mtl.diffuseTexture = texfile;
		else if (sscanf(line.c_str(), "map_Ks %511s", texfile) == 1)
			mtl.specularTexture = texfile;
		else if (sscanf(line.c_str(), "map_Ke %511s", texfile) == 1)
			mtl.emissiveTexture = texfile;
		else if (sscanf(line.c_str(), "map_bump %511s", texfile) == 1 ||
			sscanf(line.c_str(), "bump %511s", texfile) == 1 ||
			sscanf(line.c_str(), "Norm %511s", texfile) == 1)
			mtl.normalTexture = texfile;
	}
}

bool OBJLoader::parse(const char *textData, size_t textSize)
{
	positions.clear();
	texcoords.clear();
	normals.clear();
	groups.clear();
	materials.clear();
	materialLibraries.clear();
	vertexCache.clear();
	error.clear();

	currentGroup = "default";
	currentMaterialName.clear();

	LineReader reader(textData, textSize);
	std::string line;

	while (reader.nextLine(line))
	{
		char tag[32];

		if (sscanf(line.c_str(), "%31s", tag) != 1)
			continue;

		// Find the argument text after the leading tag.
		const char *rest = line.c_str();
		while (*rest != '\0' && *rest != ' ' && *rest != '\t')
			rest++;

		if (strcmp(tag, "v") == 0)
		{
			char a[64] = "", b[64] = "", c[64] = "";
			if (sscanf(rest, " %63s %63s %63s", a, b, c) != 3)
			{
				error = "OBJ vertex (v) element has fewer than 3 coordinates.";
				return false;
			}
			positions.push_back(parseVec3(a, b, c));
		}
		else if (strcmp(tag, "vt") == 0)
		{
			char a[64] = "", b[64] = "", c[64] = "";
			sscanf(rest, " %63s %63s %63s", a, b, c);
			texcoords.push_back(Vector3((float) atof(a), (float) atof(b), 0.0f));
		}
		else if (strcmp(tag, "vn") == 0)
		{
			char a[64] = "", b[64] = "", c[64] = "";
			if (sscanf(rest, " %63s %63s %63s", a, b, c) != 3)
			{
				error = "OBJ normal (vn) element has fewer than 3 coordinates.";
				return false;
			}
			normals.push_back(parseVec3(a, b, c));
		}
		else if (strcmp(tag, "f") == 0)
		{
			// Faces belong to the current group / material section. Lazily
			// create the first group and note when a new section starts.
			bool needsNewGroup = groups.empty() || currentMaterialName != groups.back().materialName ||
				currentGroup != groups.back().name;

			if (needsNewGroup)
			{
				GroupRecord groupRecord;
				groupRecord.name = currentGroup;
				groupRecord.materialName = currentMaterialName;
				groups.push_back(groupRecord);

				// The dedup cache is per group: the same p/t/n key must map
				// to the current group's vertex list.
				vertexCache.clear();
			}

			GroupRecord &group = groups.back();

			std::vector<int> facePos, faceUV, faceNorm;

			const char *cursor = rest;

			while (*cursor != '\0')
			{
				while (*cursor == ' ' || *cursor == '\t')
					cursor++;

				if (*cursor == '\0')
					break;

				int vp = 0, vt = 0, vn = 0;
				int fields = sscanf(cursor, "%d/%d/%d", &vp, &vt, &vn);

				if (fields == 1)
				{
					fields = sscanf(cursor, "%d//%d", &vp, &vn);
					vt = 0;
				}

				if (fields <= 0)
				{
					error = "OBJ face element has unparsable corner indices.";
					return false;
				}

				facePos.push_back(vp);
				faceUV.push_back(vt);
				faceNorm.push_back(vn);

				// Advance past this token.
				while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t')
					cursor++;
			}

			if (facePos.size() < 3)
			{
				error = "OBJ face element has fewer than 3 corners.";
				return false;
			}

			size_t cornerCount = facePos.size();
			std::vector<uint32> cornerIndices;
			cornerIndices.reserve(cornerCount);

			for (size_t i = 0; i < cornerCount; i++)
				cornerIndices.push_back(resolveVertex(facePos[i], faceUV[i], faceNorm[i], group));

			// Fan triangulation.
			for (size_t i = 2; i < cornerCount; i++)
			{
				group.geometry.indices.push_back(cornerIndices[0]);
				group.geometry.indices.push_back(cornerIndices[i - 1]);
				group.geometry.indices.push_back(cornerIndices[i]);
			}
		}
		else if (strcmp(tag, "g") == 0 || strcmp(tag, "o") == 0)
		{
			char name[256] = "";
			sscanf(rest, " %255s", name);
			currentGroup = name;
		}
		else if (strcmp(tag, "usemtl") == 0)
		{
			char name[256] = "";
			sscanf(rest, " %255s", name);
			currentMaterialName = name;
		}
		else if (strcmp(tag, "mtllib") == 0)
		{
			std::istringstream names(rest);
			std::string name;
			while (names >> name)
				materialLibraries.push_back(name);
		}
	}

	// Generate area-weighted normals where the OBJ omitted them.
	for (GroupRecord &group : groups)
	{
		std::vector<Vector3> generatedNormals(group.geometry.vertices.size());
		std::vector<bool> missingNormals(group.geometry.vertices.size(), false);

		for (size_t i = 0; i < group.geometry.vertices.size(); i++)
		{
			const Vertex3D &vertex = group.geometry.vertices[i];
			missingNormals[i] = vertex.normal[0] == 0.0f && vertex.normal[1] == 0.0f && vertex.normal[2] == 0.0f;
		}

		for (size_t i = 0; i + 2 < group.geometry.indices.size(); i += 3)
		{
			uint32 i0 = group.geometry.indices[i];
			uint32 i1 = group.geometry.indices[i + 1];
			uint32 i2 = group.geometry.indices[i + 2];

			const Vertex3D &v0 = group.geometry.vertices[i0];
			const Vertex3D &v1 = group.geometry.vertices[i1];
			const Vertex3D &v2 = group.geometry.vertices[i2];
			Vector3 p0(v0.position[0], v0.position[1], v0.position[2]);
			Vector3 p1(v1.position[0], v1.position[1], v1.position[2]);
			Vector3 p2(v2.position[0], v2.position[1], v2.position[2]);
			Vector3 faceNormal = (p1 - p0).cross(p2 - p0);

			if (faceNormal.lengthsq() <= 1e-12f)
				continue;

			if (missingNormals[i0])
				generatedNormals[i0] += faceNormal;
			if (missingNormals[i1])
				generatedNormals[i1] += faceNormal;
			if (missingNormals[i2])
				generatedNormals[i2] += faceNormal;
		}

		for (size_t i = 0; i < group.geometry.vertices.size(); i++)
		{
			if (!missingNormals[i])
				continue;

			Vector3 normal = generatedNormals[i].normalized();
			if (normal.lengthsq() <= 0.0f)
				normal = Vector3(0.0f, 1.0f, 0.0f);

			Vertex3D &vertex = group.geometry.vertices[i];
			vertex.normal[0] = normal.x;
			vertex.normal[1] = normal.y;
			vertex.normal[2] = normal.z;
		}
	}

	// Merge adjacent parts sharing a material to reduce draw calls.
	std::vector<GroupRecord> merged;
	for (GroupRecord &record : groups)
	{
		if (!record.geometry.vertices.empty() && !merged.empty() &&
			merged.back().materialName == record.materialName && merged.back().name == record.name)
		{
			GroupRecord &base = merged.back();
			uint32 offset = (uint32) base.geometry.vertices.size();

			base.geometry.vertices.insert(base.geometry.vertices.end(),
				record.geometry.vertices.begin(), record.geometry.vertices.end());

			for (uint32 idx : record.geometry.indices)
				base.geometry.indices.push_back(idx + offset);
		}
		else
		{
			merged.push_back(record);
		}
	}
	groups = std::move(merged);

	// Drop empty groups.
	groups.erase(std::remove_if(groups.begin(), groups.end(),
		[](const GroupRecord &g) { return g.geometry.vertices.empty(); }), groups.end());

	return true;
}

} // graphics3d
} // love
