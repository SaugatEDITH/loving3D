/**
 * LOVING3D - 3D meshes for the LOVE game engine.
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
 * A GPU-backed 3D triangle mesh. Vertices live in a single interleaved
 * vertex Buffer, triangle indices in an index Buffer, and the attribute
 * mapping is registered with love's graphics system so any renderer
 * backend (OpenGL, Vulkan, Metal) can consume them.
 *
 * The vertex layout includes a tangent frame for normal mapping:
 *   attribute 0: vec3 Position  (local space units)
 *   attribute 1: vec3 Normal    (unit length)
 *   attribute 2: vec2 Texcoord  (0..1)
 *   attribute 3: vec4 Color     (vertex color multiplier, default white)
 *
 * Under LOVING3D's right-handed coordinate system (+X right, +Y up, -Z
 * forward from the camera), front faces wind counter-clockwise as seen
 * from outside the mesh.
 **/

#pragma once

#include "Vector3.h"
#include "Matrix4.h"
#include "Material3D.h"

#include "modules/graphics/Buffer.h"
#include "modules/graphics/vertex.h"
#include "modules/graphics/Graphics.h"
#include "modules/graphics/Texture.h"
#include "modules/graphics/Shader.h"

#include "common/Object.h"

#include <vector>
#include <utility>

namespace love
{
namespace graphics3d
{

/**
 * The interleaved 3D vertex structure, matching the Shader's attribute
 * locations exactly.
 **/
struct Vertex3D
{
	float position[3];
	float normal[3];
	float texcoord[2];
	float color[4];
	float tangent[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

class Mesh3D : public love::Object
{
public:

	static love::Type type;

	/**
	 * Creates the GPU vertex Buffer + index Buffer for the given local-
	 * space triangle geometry. Geometry can be re-uploaded with
	 * setVertexData / setIndexData for dynamic updates (the buffers have a
	 * fixed capacity from this call). Indices must describe complete
	 * triangles and reference vertices in the supplied array.
	 **/
	Mesh3D(love::graphics::Graphics *gfx, const std::vector<Vertex3D> &vertices, const std::vector<uint32> &indices,
		love::graphics::BufferDataUsage usage = love::graphics::BUFFERDATAUSAGE_STATIC);
	virtual ~Mesh3D();

	size_t getVertexCount() const { return vertexCount; }
	size_t getIndexCount() const { return indexCount; }
	const std::vector<Vertex3D> &getVertexData() const { return vertexData; }

	/**
	 * Replaces the vertex/index data in the CPU cache. Call upload() (or
	 * draw, which uploads implicitly) to send to the GPU.
	 **/
	void setVertexData(const Vertex3D *data, size_t count, size_t startIndex = 0);
	void setIndexData(const uint32 *data, size_t count, size_t startIndex = 0);

	void setMaterial(Material3D *material);
	Material3D *getMaterial() const { return material.get(); }

	/**
	 * Computes the axis-aligned bounding box of all vertices in local
	 * space. The result is cached until vertex data changes. Returns false
	 * when the mesh has no vertices.
	 **/
	bool getBoundingBox(Vector3 &minBounds, Vector3 &maxBounds) const;
	bool getBounds(Vector3 &minBounds, Vector3 &maxBounds) const { return getBoundingBox(minBounds, maxBounds); }

	/**
	 * Returns whether any vertex has fractional alpha. Cached until vertex
	 * data changes.
	 **/
	bool hasTransparentVertex() const;

	/**
	 * Uploads dirty CPU-side vertex/index arrays into the GPU buffers.
	 **/
	void upload();

	/**
	 * Issues the indexed draw command for this mesh. Per-object shader
	 * uniforms and material state are the caller's (Graphics3D) job.
	 **/
	void drawBatched(love::graphics::Graphics *gfx, love::graphics::CullMode cullMode, love::graphics::Texture *texture = nullptr);
	void drawInstanced(love::graphics::Graphics *gfx, const std::vector<Matrix4> &transforms,
		love::graphics::CullMode cullMode, love::graphics::Texture *texture = nullptr);

	void addLOD(float distance, Mesh3D *mesh);
	Mesh3D *getLOD(float distance) const;

private:

	// Register the vertex attribute layout with the graphics system.
	void updateVertexAttributes(love::graphics::Graphics *gfx);
	void updateBounds() const;

	StrongRef<love::graphics::Buffer> vertexBuffer;
	StrongRef<love::graphics::Buffer> indexBuffer;

	StrongRef<Material3D> material;

	// CPU-side vertex/index caches for dynamic updates.
	std::vector<Vertex3D> vertexData;
	std::vector<uint32> indexData;
	std::vector<std::pair<float, StrongRef<Mesh3D>>> lods;

	size_t vertexCount = 0;
	size_t indexCount = 0;
	size_t stride = sizeof(Vertex3D);
	mutable Vector3 boundsMin;
	mutable Vector3 boundsMax;
	mutable bool boundsDirty = true;
	mutable bool transparencyDirty = true;
	mutable bool vertexTransparency = false;

	love::graphics::IndexDataType indexDataType = love::graphics::INDEX_UINT16;
	love::graphics::VertexAttributesID attributesID;

	bool vertexDataDirty = true;
	bool indexDataDirty = true;

}; // Mesh3D

} // graphics3d
} // love
