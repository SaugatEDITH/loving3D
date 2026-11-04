/**
 * LOVING3D - Wavefront OBJ/MTL model importer for the LOVE game engine.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org). The underlying engine
 * remains freely distributable under the terms of its original zlib license
 * (see license.txt).
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

/**
 * Minimal Wavefront OBJ model importer with MTL material file support.
 *
 * Supported OBJ elements:
 *   v, vt, vn     - positions, texcoords, normals.
 *   f             - faces with positive or negative indices, in mixed
 *                   "p", "p/t", "p//n", "p/t/n" forms. Polygons with more
 *                   than 3 corners are fan-triangulated. Missing normals
 *                   are generated from the resulting triangles.
 *   g / usemtl    - geometry groups become Model3D parts; usemtl starts a
 *                   new part so per-material draws remain separable. Adjacent
 *                   parts sharing a material are merged back together.
 *   s             - smoothing groups are ignored.
 *
 * Supported MTL elements: Kd, Ka, Ke, Ks, Ns, d, Tr, illum (ignored),
 * map_Kd, map_Ks, map_Ke, and map_bump/bump/Norm for normal maps (the
 * bump texture is wired to the shader's normal sampler). Texture filenames
 * are reported to the Lua side, which resolves them to love.graphics Image
 * objects (typically loaded via love.filesystem / love.image and cached in
 * a table keyed by filename).
 **/

#pragma once

#include "Primitives.h"
#include "Model3D.h"
#include "Material3D.h"

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace love
{
namespace graphics3d
{

class OBJLoader
{
public:

	/** Material values parsed out of an MTL file. **/
	struct MaterialRecord
	{
		std::string name;
		Vector3 diffuse = Vector3(1.0f, 1.0f, 1.0f);
		Vector3 ambient = Vector3(0.0f, 0.0f, 0.0f);
		Vector3 emissive = Vector3(0.0f, 0.0f, 0.0f);
		Vector3 specular = Vector3(0.2f, 0.2f, 0.2f);
		float shininess = 32.0f;
		float opacity = 1.0f;
		std::string diffuseTexture;  // map_Kd filename.
		std::string specularTexture; // map_Ks filename.
		std::string emissiveTexture; // map_Ke filename.
		std::string normalTexture;   // map_bump/bump/Norm filename.
	};

	/** One OBJ geometry section: a group (g) or material switch (usemtl). **/
	struct GroupRecord
	{
		std::string name;
		std::string materialName;
		PrimitiveGeometry geometry;
	};

	const std::vector<MaterialRecord> &getMaterials() const { return materials; }
	std::vector<MaterialRecord> &getMaterials() { return materials; }
	const std::vector<std::string> &getMaterialLibraries() const { return materialLibraries; }

	const std::vector<GroupRecord> &getGroups() const { return groups; }
	std::vector<GroupRecord> &getGroups() { return groups; }

	const std::string &getError() const { return error; }

	/**
	 * Parses OBJ source text. Returns false on a fatal parse error (see
	 * getError() for a message).
	 **/
	bool parse(const char *textData, size_t textSize);

	/** Parses MTL content and appends MaterialRecords. **/
	void parseMTL(const char *content, size_t contentSize);

	/**
	 * Builds GPU resources from the parsed OBJ data. Each group becomes a
	 * Mesh3D part of the returned Model3D.
	 *
	 * The materialBinder callback is invoked once per distinct material name
	 * (name in, Material3D out, or null for a default material); the Lua
	 * side uses this to look up a Lua-built Material (optionally configured
	 * with loaded images) or to create new ones, adopting the record values
	 * via getMaterials().
	 *
	 * Returns null when no groups had vertices.
	 **/
	Model3D *createModel(love::graphics::Graphics *gfx, love::graphics::BufferDataUsage usage,
		const std::function<Material3D *(const MaterialRecord &)> &materialBinder);

private:

	// Resolves one OBJ vertex triplet ("p", "p/t", ...) into the current
	// group's geometry, deduplicating identical triplets.
	uint32 resolveVertex(int pIndex, int tIndex, int nIndex, GroupRecord &group);

	std::vector<MaterialRecord> materials;
	std::vector<std::string> materialLibraries;
	std::vector<GroupRecord> groups;
	std::string error;

	// Raw OBJ element pools (1-based indexing, negative = from end).
	std::vector<Vector3> positions;
	std::vector<Vector3> texcoords;
	std::vector<Vector3> normals;

	std::string currentGroup;
	std::string currentMaterialName;

	// Deduplication of (position, uv, normal) triplets per group. Keys are
	// "p/t/n" strings, matching OBJ indexing conventions.
	std::map<std::string, uint32> vertexCache;

}; // OBJLoader

} // graphics3d
} // love
