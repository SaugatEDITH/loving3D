/**
 * LOVING3D - native 3D rendering module for the LOVE game engine.
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
 * The love.graphics3d module.
 *
 * COORDINATE CONVENTIONS (LOVING3D uses a right-handed 3D system):
 *   +X is right, +Y is up, +Z points toward the viewer. The default camera
 *   looks down -Z. All positions/extent values are in world units.
 *
 * INTEGRATION MODEL:
 *   This module is a thin scene layer on top of love.graphics' public
 *   graphics API. Resources (vertex buffers, index buffers, shaders, the
 *   GPU draw commands) are created and consumed through the backend-
 *   independent love::graphics::Graphics interface, so 3D ends up rendering
 *   through the same OpenGL / Vulkan / Metal backends used by 2D drawing.
 *
 * PASS MODEL:
 *   Lua calls begin3d() to enter a 3D scene, then draws objects (they are
 *   queued in this module), then calls end3d() to flush the queue and
 *   restore the 2D graphics state. Internally we push the graphics state
 *   (render targets, depth state, culling, winding, shader) and pop it on
 *   the way out, so 2D and 3D drawing interleave cleanly.
 *
 * SHADER:
 *   A single Blinn-Phong lit shader is compiled on the first begin3d,
 *   through Graphics::newShader (glslang-based, so all three backends get
 *   SPIR-V on Vulkan/Metal automatically). Unlit materials are a material
 *   flag, not a second shader.
 *
 * DEPTH / CULLING:
 *   Depth-tested (LEQUAL) with depth writes; backface culling comes from
 *   each material (default CULL_BACK). When drawing to a Canvas without a
 *   depth attachment, a temporary depth buffer is attached. The main
 *   window requires t.window.depth = true (see conf.lua) for depth
 *   drawing, enforced with clear error messages.
 *
 * LIGHTS:
 *   Up to LOVE3D_MAX_LIGHTS lights (directional / point / spot) are
 *   sampled per draw; see Light3D.
 **/

#pragma once

#include "common/Module.h"
#include "common/Object.h"

#include "Camera3D.h"
#include "Light3D.h"
#include "Material3D.h"
#include "Mesh3D.h"
#include "Model3D.h"
#include "Node3D.h"
#include "OBJLoader.h"
#include "Primitives.h"

#include "modules/graphics/Graphics.h"
#include "modules/graphics/Shader.h"

#include <functional>
#include <string>
#include <vector>

namespace love
{
namespace graphics3d
{

// The max number of simultaneously active lights; grow the shader uniform
// arrays if this changes.
const int LOVE3D_MAX_LIGHTS = 8;

class Graphics3D : public love::Module
{
public:

	static love::Type type;

	Graphics3D();

	virtual ~Graphics3D();

	/**
	 * Resource creation. All types are plain love Objects usable from Lua.
	 **/
	Camera3D *newCamera();
	Material3D *newMaterial();
	void setMaterialFragmentFunction(Material3D *material, const std::string &source);
	Light3D *newLight(Light3D::LightType lightType);

	Mesh3D *newMesh(const std::vector<Vertex3D> &vertices, const std::vector<uint32> &indices,
		love::graphics::BufferDataUsage usage = love::graphics::BUFFERDATAUSAGE_STATIC);

	// Primitive generators (also exposed by name from Lua).
	Mesh3D *newCube();
	Mesh3D *newBox(float width, float height, float depth);
	Mesh3D *newSphere(int latitudes, int longitudes, float radius);
	Mesh3D *newPlane(int subdivisions, float size);
	Mesh3D *newCylinder(int slices, float radius, float height);
	Mesh3D *newCone(int slices, float radius, float height);
	Mesh3D *newCapsule(int slices, int rings, float radius, float height);
	Mesh3D *newTorus(int segments, int rings, float majorRadius, float minorRadius);

	/**
	 * Builds a Model3D out of a parsed OBJ (see OBJLoader). materialBinder
	 * may return a Material3D per material (Lua supplies textures), or null
	 * to build flat default materials from the MTL values.
	 **/
	Model3D *newModel(OBJLoader &loader, love::graphics::BufferDataUsage usage,
		const std::function<Material3D *(const OBJLoader::MaterialRecord &)> &materialBinder);

	// Scene configuration.
	void setCamera(Camera3D *camera);
	Camera3D *getCamera() const { return camera.get(); }

	void addLight(Light3D *light);
	void removeLight(Light3D *light);
	void clearLights();
	const std::vector<StrongRef<Light3D>> &getLights() const { return lights; }

	void setAmbientColor(float r, float g, float b);
	Vector3 getAmbientColor() const { return ambientColor; }

	/**
	 * 3D pass. While in3D, draws are queued and flushed once on end3D.
	 * Opaque draws are submitted first; transparent draws use back-to-front
	 * sorting and disable depth writes for the transparent pass.
	 **/
	bool begin3D();
	void end3D();
	bool isDrawing() const { return in3D; }

	void drawMesh(Mesh3D *mesh, const Matrix4 &modelMatrix);
	void drawInstanced(Mesh3D *mesh, const std::vector<Matrix4> &transforms);
	void drawModel(Model3D *model, const Matrix4 &modelMatrix);
	void drawNode(Node3D *node);
	void debugLine(const Vector3 &from, const Vector3 &to, const Vector4 &color, bool depthTest);
	void debugBox(const Vector3 &boundsMin, const Vector3 &boundsMax, const Vector4 &color, bool depthTest);
	void debugSphere(const Vector3 &center, float radius, const Vector4 &color, bool depthTest);
	void debugNormals(Mesh3D *mesh, const Matrix4 &modelMatrix, float length, bool depthTest);

	/**
	 * Clears the active render target's depth buffer (1.0 = far). Useful to
	 * layer 2D + 3D draws across slices of a frame.
	 **/
	void clearDepth();

	/**
	 * Front-face winding tweak. CCW is correct for LOVING3D-generated
	 * meshes and clean OBJ imports; use CW to flip a mesh whose source data
	 * was authored mirrored.
	 **/
	void setWinding(love::graphics::Winding winding);
	love::graphics::Winding getWinding() const { return winding; }
	void setAutoBatching(bool enabled) { autoBatching = enabled; }
	bool isAutoBatchingEnabled() const { return autoBatching; }

	/**
	 * Module-level stats (3D draws passed through since the last reset, plus
	 * live counts of resources created here).
	 **/
	struct Stats3D
	{
		int drawsPassed = 0;      // draw() calls this frame.
		int drawsFlushed = 0;     // Cumulative GPU draws.
		int queued = 0;
		int drawCalls = 0;
		int triangles = 0;
		int culled = 0;
		int lightsUsed = 0;
	};

	Stats3D getStats() const;

private:

	struct UniformInfos;

	// Lazily compiles the Blinn-Phong lit shader on first use.
	void ensureShader();

	// Scene-level uniforms (camera, ambient, lights) — once per begin3D.
	void updateSceneUniforms(love::graphics::Shader *shader, const UniformInfos &uniforms);

	// Queues get flushed once at end3D.
	void flushQueuedDraws();

	// Pack one draw's per-object uniforms (model/normal/material) and mesh
	// state, then issue the indexed draw through the graphics backend.
	void drawObject(Mesh3D *mesh, const Matrix4 &modelMatrix, Material3D *material,
		const std::vector<Matrix4> *instances = nullptr);

	love::graphics::Graphics *getGraphics() const;

	StrongRef<love::graphics::Shader> litShader;
	StrongRef<love::graphics::Shader> instancedLitShader;

	struct UniformInfos
	{
		const love::graphics::Shader::UniformInfo *projectionMatrix = nullptr;
		const love::graphics::Shader::UniformInfo *viewMatrix = nullptr;
		const love::graphics::Shader::UniformInfo *cameraPos = nullptr;
		const love::graphics::Shader::UniformInfo *ambientLight = nullptr;
		const love::graphics::Shader::UniformInfo *lightCount = nullptr;
		const love::graphics::Shader::UniformInfo *lightPos = nullptr;
		const love::graphics::Shader::UniformInfo *lightDir = nullptr;
		const love::graphics::Shader::UniformInfo *lightParam = nullptr;
		const love::graphics::Shader::UniformInfo *lightColor = nullptr;
		const love::graphics::Shader::UniformInfo *modelMatrix = nullptr;
		const love::graphics::Shader::UniformInfo *normalMatrix = nullptr;
		const love::graphics::Shader::UniformInfo *materialColor = nullptr;
		const love::graphics::Shader::UniformInfo *materialSpec = nullptr;
		const love::graphics::Shader::UniformInfo *materialEmissive = nullptr;
		const love::graphics::Shader::UniformInfo *materialAlpha = nullptr;
		const love::graphics::Shader::UniformInfo *diffuseTexture = nullptr;
		const love::graphics::Shader::UniformInfo *normalTexture = nullptr;
		const love::graphics::Shader::UniformInfo *specularTexture = nullptr;
		const love::graphics::Shader::UniformInfo *emissiveTexture = nullptr;
	};

	UniformInfos uniforms;
	UniformInfos instancedUniforms;
	void cacheUniformInfos(love::graphics::Shader *shader, UniformInfos &target);

	// Used when a mesh has no material assigned.
	StrongRef<Material3D> fallbackMaterial;

	struct QueuedDraw
	{
		StrongRef<Mesh3D> mesh;
		StrongRef<Material3D> material;
		Matrix4 modelMatrix;
		bool debug = false;
		bool depthTest = true;
		bool explicitInstanced = false;
	};
	friend class Node3D;

	std::vector<QueuedDraw> drawQueue;
	StrongRef<Mesh3D> debugBoxMesh;

	StrongRef<Camera3D> camera;
	std::vector<StrongRef<Light3D>> lights;
	Vector3 ambientColor = Vector3(0.15f, 0.15f, 0.15f);

	bool in3D = false;
	bool autoBatching = true;

	love::graphics::Winding winding = love::graphics::WINDING_CCW;

	// Scratch arrays uploaded as light uniform data.
	Vector4 lightPosData[LOVE3D_MAX_LIGHTS];
	Vector4 lightDirData[LOVE3D_MAX_LIGHTS];
	Vector4 lightParamData[LOVE3D_MAX_LIGHTS];
	Vector4 lightColorData[LOVE3D_MAX_LIGHTS];

	Stats3D stats;

}; // Graphics3D

} // graphics3d
} // love
