/**
 * LOVING3D - native 3D rendering module implementation.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org). The underlying engine
 * remains freely distributable under the terms of its original zlib license
 * (see license.txt).
 **/

#include "Graphics3D.h"

#include "common/Exception.h"

#include <cmath>
#include <cstring>
#include <algorithm>
#include <functional>
#include <string>

namespace love
{
namespace graphics3d
{

love::Type Graphics3D::type("Graphics3D", &Module::type);

static bool isUniformUsable(const love::graphics::Shader::UniformInfo *uniform)
{
	return uniform != nullptr && uniform->active && uniform->data != nullptr;
}

Graphics3D::Graphics3D()
	: Module(M_GRAPHICS3D, "love.graphics3d")
{
}

Graphics3D::~Graphics3D()
{
}

love::graphics::Graphics *Graphics3D::getGraphics() const
{
	love::graphics::Graphics *gfx = Module::getInstance<love::graphics::Graphics>(M_GRAPHICS);

	if (gfx == nullptr)
		throw love::Exception("love.graphics must be initialized before love.graphics3d can be used.");

	return gfx;
}

Camera3D *Graphics3D::newCamera()
{
	return new Camera3D();
}

Material3D *Graphics3D::newMaterial()
{
	return new Material3D();
}

Light3D *Graphics3D::newLight(Light3D::LightType lightType)
{
	return new Light3D(lightType);
}

Mesh3D *Graphics3D::newMesh(const std::vector<Vertex3D> &vertices, const std::vector<uint32> &indices, love::graphics::BufferDataUsage usage)
{
	love::graphics::Graphics *gfx = getGraphics();
	return new Mesh3D(gfx, vertices, indices, usage);
}

Mesh3D *Graphics3D::newCube()
{
	PrimitiveGeometry geo = primitives::cube();
	return newMesh(geo.vertices, geo.indices);
}

Mesh3D *Graphics3D::newBox(float width, float height, float depth)
{
	PrimitiveGeometry geo = primitives::box(width, height, depth);
	return newMesh(geo.vertices, geo.indices);
}

Mesh3D *Graphics3D::newSphere(int latitudes, int longitudes, float radius)
{
	PrimitiveGeometry geo = primitives::sphere(latitudes, longitudes, radius);
	return newMesh(geo.vertices, geo.indices);
}

Mesh3D *Graphics3D::newPlane(int subdivisions, float size)
{
	PrimitiveGeometry geo = primitives::plane(subdivisions, size);
	return newMesh(geo.vertices, geo.indices);
}

Mesh3D *Graphics3D::newCylinder(int slices, float radius, float height)
{
	PrimitiveGeometry geo = primitives::cylinder(slices, radius, height);
	return newMesh(geo.vertices, geo.indices);
}

Mesh3D *Graphics3D::newCone(int slices, float radius, float height)
{
	PrimitiveGeometry geo = primitives::cone(slices, radius, height);
	return newMesh(geo.vertices, geo.indices);
}

Mesh3D *Graphics3D::newCapsule(int slices, int rings, float radius, float height)
{
	PrimitiveGeometry geo = primitives::capsule(slices, rings, radius, height);
	return newMesh(geo.vertices, geo.indices);
}

Mesh3D *Graphics3D::newTorus(int segments, int rings, float majorRadius, float minorRadius)
{
	PrimitiveGeometry geo = primitives::torus(segments, rings, majorRadius, minorRadius);
	return newMesh(geo.vertices, geo.indices);
}

Model3D *Graphics3D::newModel(OBJLoader &loader, love::graphics::BufferDataUsage usage,
	const std::function<Material3D *(const OBJLoader::MaterialRecord &)> &materialBinder)
{
	love::graphics::Graphics *gfx = getGraphics();

	Model3D *model = loader.createModel(gfx, usage, materialBinder);

	if (model == nullptr)
		throw love::Exception("OBJ model had no drawable geometry.");

	return model;
}

void Graphics3D::setCamera(Camera3D *camera_)
{
	if (camera_ == nullptr)
		throw love::Exception("Expected a Camera3D argument.");

	camera.set(camera_);
}

void Graphics3D::addLight(Light3D *light)
{
	if (light == nullptr)
		throw love::Exception("Expected a Light3D argument.");

	for (const auto &existing : lights)
	{
		if (existing.get() == light)
			return;
	}

	if ((int) lights.size() >= LOVE3D_MAX_LIGHTS)
		throw love::Exception("Too many active lights (max %d).", LOVE3D_MAX_LIGHTS);

	lights.emplace_back(light);
}

void Graphics3D::removeLight(Light3D *light)
{
	for (auto it = lights.begin(); it != lights.end(); ++it)
	{
		if (it->get() == light)
		{
			lights.erase(it);
			return;
		}
	}
}

void Graphics3D::clearLights()
{
	lights.clear();
}

void Graphics3D::setAmbientColor(float r, float g, float b)
{
	ambientColor = Vector3(r, g, b);
}

void Graphics3D::setWinding(love::graphics::Winding winding_)
{
	winding = winding_;
}

Graphics3D::Stats3D Graphics3D::getStats() const
{
	return stats;
}

/**
 * GLSL sources for the LOVING3D standard Blinn-Phong shader.
 *
 * These use LOVE's custom shader dialect with raw entry points
 * (void vertexmain() / void pixelmain()); LOVE's wrapper applies the
 * clip-space transform (Y flip when rendering to a Canvas, Z range fixup
 * for Vulkan/Metal) automatically after vertexmain writes love_Position.
 *
 * Attribute locations must match the Vertex3D layout registered by
 * Mesh3D::updateVertexAttributes (locations 0..3).
 **/

static const char *standard3DVertexSource = R"(
// LOVING3D standard 3D vertex shader.

layout (location = 0) in vec3 love3d_VertexPosition;
layout (location = 1) in vec3 love3d_VertexNormal;
layout (location = 2) in vec2 love3d_VertexTexCoord;
layout (location = 3) in vec4 love3d_VertexColor;
layout (location = 4) in vec4 love3d_VertexTangent;

uniform mat4 love3d_ProjectionMatrix;
uniform mat4 love3d_ViewMatrix;
uniform mat4 love3d_ModelMatrix;
uniform mat4 love3d_NormalMatrix;

out vec3 love3d_VaryWorldPos;
out vec3 love3d_VaryNormal;
out vec2 love3d_VaryTexCoord;
out vec4 love3d_VaryColor;
out vec4 love3d_VaryTangent;

void vertexmain()
{
	vec4 worldPosition = love3d_ModelMatrix * vec4(love3d_VertexPosition, 1.0);
	love3d_VaryWorldPos = worldPosition.xyz;

	// The normal matrix is the inverse-transpose of the model matrix
	// (rotation part), uploaded from C++.
	love3d_VaryNormal = (love3d_NormalMatrix * vec4(love3d_VertexNormal, 0.0)).xyz;

	love3d_VaryTexCoord = love3d_VertexTexCoord;
	love3d_VaryColor = love3d_VertexColor;
	love3d_VaryTangent = vec4(
		(love3d_NormalMatrix * vec4(love3d_VertexTangent.xyz, 0.0)).xyz,
		love3d_VertexTangent.w);

	// Right-handed clip space: +Y up, camera looks down -Z. LOVE's
	// wrapper then applies the target-space transform (Y flip for Canvas
	// targets, Z remap for Vulkan/Metal).
	love_Position = love3d_ProjectionMatrix * (love3d_ViewMatrix * worldPosition);
}
)";

static const char *standard3DPixelSource = R"(
// LOVING3D standard 3D pixel shader: Blinn-Phong lighting with
// directional, point and spot lights, optional normal/specular/emissive
// texture maps, and an unlit material mode.

layout (location = 0) out vec4 love3d_OutColor;

in vec3 love3d_VaryWorldPos;
in vec3 love3d_VaryNormal;
in vec2 love3d_VaryTexCoord;
in vec4 love3d_VaryColor;
in vec4 love3d_VaryTangent;

#define LOVE3D_MAX_LIGHTS 8

uniform vec4 love3d_MaterialColor;   // rgb = diffuse tint, a = opacity
uniform vec4 love3d_MaterialSpec;    // x = specular strength, y = shininess, z = unlit flag, w = has normal map
uniform vec4 love3d_MaterialEmissive;
uniform vec2 love3d_MaterialAlpha;   // x = cutoff, y = alpha-mask flag

uniform vec3 love3d_CameraPos;
uniform vec3 love3d_AmbientLight;
uniform int love3d_LightCount;

uniform vec4 love3d_LightPos[LOVE3D_MAX_LIGHTS];   // xyz = position, w = intensity
uniform vec4 love3d_LightDir[LOVE3D_MAX_LIGHTS];   // xyz = direction light travels, w = range
uniform vec4 love3d_LightParam[LOVE3D_MAX_LIGHTS]; // x = type (0 dir, 1 point, 2 spot), y = outer angle, z = inner angle, w = falloff
uniform vec4 love3d_LightColor[LOVE3D_MAX_LIGHTS]; // rgb = color

uniform Image love3d_DiffuseTex;
uniform Image love3d_NormalTex;
uniform Image love3d_SpecularTex;
uniform Image love3d_EmissiveTex;

vec4 love3d_CustomFragment(vec4 color)
{
	return color;
}

void pixelmain()
{
	vec4 texel = Texel(love3d_DiffuseTex, love3d_VaryTexCoord);
	if (love3d_MaterialAlpha.y > 0.5 && texel.a * love3d_MaterialColor.a * love3d_VaryColor.a < love3d_MaterialAlpha.x)
		discard;

	// Material and light RGB inputs are converted to linear light here.
	// Ambient RGB is already treated as linear; framebuffer output encoding
	// follows LOVE's gamma-correct rendering pipeline.
	vec3 albedo = gammaToLinear(love3d_MaterialColor.rgb) * texel.rgb * love3d_VaryColor.rgb;

	if (love3d_MaterialSpec.z > 0.5)
	{
		// Unlit: plain texel * tint (plus emissive).
		vec3 unlit = albedo + gammaToLinear(love3d_MaterialEmissive.rgb);
		love3d_OutColor = love3d_CustomFragment(vec4(unlit, texel.a * love3d_MaterialColor.a * love3d_VaryColor.a));
		return;
	}

	vec3 geometricNormal = normalize(love3d_VaryNormal);

	vec3 normal = geometricNormal;

	if (love3d_MaterialSpec.w > 0.5)
	{
		// Tangent frame is imported from glTF or generated from geometry and UVs.
		vec3 tangentMap = Texel(love3d_NormalTex, love3d_VaryTexCoord).rgb;
		tangentMap = tangentMap * 2.0 - 1.0;
		vec3 tangent = normalize(love3d_VaryTangent.xyz
			- geometricNormal * dot(geometricNormal, love3d_VaryTangent.xyz));
		vec3 bitangent = normalize(cross(geometricNormal, tangent)) * love3d_VaryTangent.w;
		mat3 tbn = mat3(tangent, bitangent, geometricNormal);

		normal = normalize(tbn * tangentMap);
	}

	float specStrength = love3d_MaterialSpec.x;
	float shininess = max(love3d_MaterialSpec.y, 1.0);

	vec3 totalDiffuse = vec3(0.0);
	vec3 totalSpecular = vec3(0.0);

	for (int i = 0; i < love3d_LightCount && i < LOVE3D_MAX_LIGHTS; i++)
	{
		vec4 lightPosition = love3d_LightPos[i];
		vec4 lightDirection = love3d_LightDir[i];
		vec4 lightParams = love3d_LightParam[i];
		vec3 lightColor = gammaToLinear(love3d_LightColor[i].rgb) * lightPosition.w;

		float lightType = lightParams.x; // 0 directional, 1 point, 2 spot.

		vec3 toLight;
		float attenuation;

		if (lightType < 0.5)
		{
			// Directional light: direction is where the light shines.
			toLight = -lightDirection.xyz;
			attenuation = 1.0;
		}
		else
		{
			vec3 fragToLight = lightPosition.xyz - love3d_VaryWorldPos;
			float dist = length(fragToLight);
			toLight = fragToLight / max(dist, 1e-5);

			// Linear falloff to zero at the light's range, shaped by the
			// falloff exponent (0 = no distance falloff).
			float falloff = clamp(1.0 - dist / max(lightDirection.w, 1e-5), 0.0, 1.0);
			attenuation = pow(falloff, max(lightParams.w, 0.0));

			if (lightType > 1.5)
			{
				// Spot cone: fully lit inside the inner angle, smooth
				// fade to the outer angle.
				float outerCos = cos(lightParams.y);
				float innerCos = cos(lightParams.z);
				float cosToLight = dot(toLight, -lightDirection.xyz);

				float t = clamp((cosToLight - outerCos) / max(innerCos - outerCos, 1e-6), 0.0, 1.0);
				attenuation *= t * t * (3.0 - 2.0 * t);
			}
		}

		float nl = max(dot(normal, toLight), 0.0);

		if (nl > 0.0)
		{
			totalDiffuse += lightColor * (attenuation * nl);

			if (specStrength > 0.0)
			{
				vec3 viewDir = normalize(love3d_CameraPos - love3d_VaryWorldPos);
				vec3 halfDir = normalize(toLight + viewDir);
				float specAngle = max(dot(normal, halfDir), 0.0);
				totalSpecular += lightColor * (attenuation * pow(specAngle, shininess) * specStrength);
			}
		}
	}

	// Specular map: default texture is white, so a missing map is a
	// no-op multiplier.
	vec3 specMap = Texel(love3d_SpecularTex, love3d_VaryTexCoord).rgb;

	vec3 color = (love3d_AmbientLight + totalDiffuse) * albedo;
	color += totalSpecular * specMap;

	// Emissive: material color * emissive texture (default texture is
	// white).
	color += gammaToLinear(love3d_MaterialEmissive.rgb) * Texel(love3d_EmissiveTex, love3d_VaryTexCoord).rgb;

	love3d_OutColor = love3d_CustomFragment(vec4(color, texel.a * love3d_MaterialColor.a * love3d_VaryColor.a));
}
)";

static std::string makeInstancedVertexSource()
{
	std::string source = standard3DVertexSource;
	auto replaceAll = [](std::string &text, const std::string &from, const std::string &to)
	{
		size_t position = 0;
		while ((position = text.find(from, position)) != std::string::npos)
		{
			text.replace(position, from.length(), to);
			position += to.length();
		}
	};
	replaceAll(source, "uniform mat4 love3d_ModelMatrix;",
		"layout (location = 5) in mat4 love3d_InstanceMatrix;");
	replaceAll(source, "love3d_ModelMatrix", "love3d_InstanceMatrix");
	replaceAll(source, "(love3d_NormalMatrix * vec4(love3d_VertexNormal, 0.0)).xyz",
		"(transpose(inverse(mat3(love3d_InstanceMatrix))) * love3d_VertexNormal)");
	replaceAll(source, "(love3d_NormalMatrix * vec4(love3d_VertexTangent.xyz, 0.0)).xyz",
		"(transpose(inverse(mat3(love3d_InstanceMatrix))) * love3d_VertexTangent.xyz)");
	return source;
}

void Graphics3D::setMaterialFragmentFunction(Material3D *material, const std::string &source)
{
	if (material == nullptr)
		throw love::Exception("Expected a Material3D.");
	if (in3D)
		throw love::Exception("3D material shaders must be changed outside the 3D render pass.");

	if (source.empty())
	{
		material->setFragmentFunction("");
		material->setShader(nullptr);
		material->setInstancedShader(nullptr);
		return;
	}

	const std::string defaultFunction = "vec4 love3d_CustomFragment(vec4 color)\n{\n\treturn color;\n}";
	std::string pixelSource = standard3DPixelSource;
	size_t hook = pixelSource.find(defaultFunction);
	if (hook == std::string::npos)
		throw love::Exception("Could not locate the LOVING3D fragment injection point.");
	pixelSource.replace(hook, defaultFunction.length(), source);

	std::vector<std::string> stages = { standard3DVertexSource, pixelSource };
	love::graphics::Shader::CompileOptions options;
	options.debugName = "LOVING3D material fragment";
	StrongRef<love::graphics::Shader> shader(getGraphics()->newShader(stages, options), Acquire::NORETAIN);
	std::vector<std::string> instancedStages = { makeInstancedVertexSource(), pixelSource };
	options.debugName = "LOVING3D material fragment instanced";
	StrongRef<love::graphics::Shader> instancedShader(getGraphics()->newShader(instancedStages, options), Acquire::NORETAIN);
	material->setFragmentFunction(source);
	material->setShader(shader.get());
	material->setInstancedShader(instancedShader.get());
}

void Graphics3D::ensureShader()
{
	if (litShader.get() != nullptr && instancedLitShader.get() != nullptr)
		return;

	love::graphics::Graphics *gfx = getGraphics();

	std::vector<std::string> stages;
	stages.push_back(standard3DVertexSource);
	stages.push_back(standard3DPixelSource);

	love::graphics::Shader::CompileOptions options;
	options.debugName = "LOVING3D standard lit";

	if (litShader.get() == nullptr)
	{
		litShader.set(gfx->newShader(stages, options), Acquire::NORETAIN);
		cacheUniformInfos(litShader.get(), uniforms);
	}

	if (instancedLitShader.get() == nullptr)
	{
		std::vector<std::string> instancedStages;
		instancedStages.push_back(makeInstancedVertexSource());
		instancedStages.push_back(standard3DPixelSource);
		options.debugName = "LOVING3D standard lit instanced";
		instancedLitShader.set(gfx->newShader(instancedStages, options), Acquire::NORETAIN);
		cacheUniformInfos(instancedLitShader.get(), instancedUniforms);
	}
}

void Graphics3D::cacheUniformInfos(love::graphics::Shader *shader, UniformInfos &target)
{
	auto getUniform = [shader](const char *name) -> const love::graphics::Shader::UniformInfo *
	{
		return shader->getUniformInfo(name);
	};
	target.projectionMatrix = getUniform("love3d_ProjectionMatrix");
	target.viewMatrix = getUniform("love3d_ViewMatrix");
	target.cameraPos = getUniform("love3d_CameraPos");
	target.ambientLight = getUniform("love3d_AmbientLight");
	target.lightCount = getUniform("love3d_LightCount");
	target.lightPos = getUniform("love3d_LightPos");
	target.lightDir = getUniform("love3d_LightDir");
	target.lightParam = getUniform("love3d_LightParam");
	target.lightColor = getUniform("love3d_LightColor");
	target.modelMatrix = getUniform("love3d_ModelMatrix");
	target.normalMatrix = getUniform("love3d_NormalMatrix");
	target.materialColor = getUniform("love3d_MaterialColor");
	target.materialSpec = getUniform("love3d_MaterialSpec");
	target.materialEmissive = getUniform("love3d_MaterialEmissive");
	target.materialAlpha = getUniform("love3d_MaterialAlpha");
	target.diffuseTexture = getUniform("love3d_DiffuseTex");
	target.normalTexture = getUniform("love3d_NormalTex");
	target.specularTexture = getUniform("love3d_SpecularTex");
	target.emissiveTexture = getUniform("love3d_EmissiveTex");
}

bool Graphics3D::begin3D()
{
	if (in3D)
		throw love::Exception("begin3D was already called. Call end3d first.");

	love::graphics::Graphics *gfx = getGraphics();

	// Compile shaders on first use (may throw with informative compile
	// errors; that's better surfaced before the state push).
	ensureShader();

	gfx->push(love::graphics::Graphics::STACK_ALL);

	try
	{
		// Flush any in-progress 2D batched draws before we touch state.
		gfx->flushBatchedDraws();

		if (gfx->isRenderTargetActive())
		{
			// Attach a temporary depth buffer when the Canvas doesn't
			// already have one, so depth testing works on Canvas targets.
			auto rts = gfx->getRenderTargets();

			love::graphics::Texture *dstexture = rts.depthStencil.texture;
			bool hasDepth = dstexture != nullptr && isPixelFormatDepth(dstexture->getPixelFormat());

			if (!hasDepth && (rts.temporaryRTFlags & love::graphics::Graphics::TEMPORARY_RT_DEPTH) == 0)
			{
				love::graphics::Graphics::RenderTargetsStrongRef updated;
				updated.colors.reserve(rts.colors.size());

				for (const auto &rt : rts.colors)
					updated.colors.emplace_back(rt.texture, rt.slice, rt.mipmap);

				updated.depthStencil = love::graphics::Graphics::RenderTargetStrongRef(
					rts.depthStencil.texture, rts.depthStencil.slice, rts.depthStencil.mipmap);
				updated.temporaryRTFlags = rts.temporaryRTFlags | love::graphics::Graphics::TEMPORARY_RT_DEPTH;
				updated.shaderLayer = rts.shaderLayer;

				gfx->setRenderTargets(updated);
			}
		}

		// Depth-only clear to the far plane.
		gfx->clear(love::graphics::OptionalColorD(), OptionalInt(), OptionalDouble(1.0));

		// Depth-test + write. This throws an informative error if the
		// window has no depth buffer (set t.window.depth = true).
		gfx->setDepthMode(love::graphics::COMPARE_LEQUAL, true);

		gfx->setFrontFaceWinding(winding);

		// Bind the 3D shader for the pass.
		gfx->setShader(litShader.get());

		updateSceneUniforms(litShader.get(), uniforms);
	}
	catch (...)
	{
		gfx->pop();
		throw;
	}

	drawQueue.clear();
	stats.drawsPassed = 0;
	stats.drawCalls = 0;
	stats.triangles = 0;
	stats.culled = 0;
	stats.lightsUsed = (int) std::min(lights.size(), (size_t) LOVE3D_MAX_LIGHTS);
	stats.queued = 0;

	in3D = true;
	return true;
}

void Graphics3D::updateSceneUniforms(love::graphics::Shader *shader, const UniformInfos &activeUniforms)
{
	// Camera matrices. Without a camera, fall back to a sensible default
	// perspective (so users can draw before configuring a camera).
	Matrix4 projectionMatrix;
	Matrix4 viewMatrix;
	Vector3 cameraPos;

	if (camera.get() != nullptr)
	{
		projectionMatrix = camera->getProjectionMatrix();
		viewMatrix = camera->getViewMatrix();
		cameraPos = camera->getPosition();
	}
	else
	{
		projectionMatrix = Matrix4::perspective(70.0f * 3.14159265359f / 180.0f, 1.0f, 0.1f, 1000.0f);
		viewMatrix = Matrix4::identity();
		cameraPos = Vector3(0.0f, 0.0f, 0.0f);
	}

	if (isUniformUsable(activeUniforms.projectionMatrix))
	{
		memcpy(activeUniforms.projectionMatrix->data, projectionMatrix.m, sizeof(float) * 16);
		shader->updateUniform(activeUniforms.projectionMatrix, 1);
	}

	if (isUniformUsable(activeUniforms.viewMatrix))
	{
		memcpy(activeUniforms.viewMatrix->data, viewMatrix.m, sizeof(float) * 16);
		shader->updateUniform(activeUniforms.viewMatrix, 1);
	}

	if (isUniformUsable(activeUniforms.cameraPos))
	{
		float pos[3] = { cameraPos.x, cameraPos.y, cameraPos.z };
		memcpy(activeUniforms.cameraPos->data, pos, sizeof(float) * 3);
		shader->updateUniform(activeUniforms.cameraPos, 1);
	}

	if (isUniformUsable(activeUniforms.ambientLight))
	{
		float ambient[3] = { ambientColor.x, ambientColor.y, ambientColor.z };
		memcpy(activeUniforms.ambientLight->data, ambient, sizeof(float) * 3);
		shader->updateUniform(activeUniforms.ambientLight, 1);
	}

	// Lights.
	int lightCount = (int) std::min(lights.size(), (size_t) LOVE3D_MAX_LIGHTS);

	for (int i = 0; i < LOVE3D_MAX_LIGHTS; i++)
	{
		lightPosData[i] = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		lightDirData[i] = Vector4(0.0f, -1.0f, 0.0f, 10.0f);
		lightParamData[i] = Vector4(0.0f, 1.0f, 1.0f, 1.0f);
		lightColorData[i] = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	for (int i = 0; i < lightCount; i++)
	{
		Light3D *light = lights[i].get();

		lightPosData[i] = Vector4(light->getPosition(), light->getIntensity());
		lightDirData[i] = Vector4(light->getNormalizedDirection(), light->getRange());

		float outerCos = cosf(light->getSpotOuterDegrees() * 3.14159265359f / 180.0f);
		float innerCos = cosf(light->getSpotInnerDegrees() * 3.14159265359f / 180.0f);

		lightParamData[i] = Vector4((float) light->getType(), outerCos, innerCos, light->getFalloff());
		lightColorData[i] = Vector4(light->getColor(), 0.0f);
	}

	if (isUniformUsable(activeUniforms.lightCount))
	{
		activeUniforms.lightCount->ints[0] = lightCount;
		shader->updateUniform(activeUniforms.lightCount, 1);
	}

	if (isUniformUsable(activeUniforms.lightPos))
	{
		memcpy(activeUniforms.lightPos->data, lightPosData, sizeof(Vector4) * LOVE3D_MAX_LIGHTS);
		shader->updateUniform(activeUniforms.lightPos, LOVE3D_MAX_LIGHTS);
	}

	if (isUniformUsable(activeUniforms.lightDir))
	{
		memcpy(activeUniforms.lightDir->data, lightDirData, sizeof(Vector4) * LOVE3D_MAX_LIGHTS);
		shader->updateUniform(activeUniforms.lightDir, LOVE3D_MAX_LIGHTS);
	}

	if (isUniformUsable(activeUniforms.lightParam))
	{
		memcpy(activeUniforms.lightParam->data, lightParamData, sizeof(Vector4) * LOVE3D_MAX_LIGHTS);
		shader->updateUniform(activeUniforms.lightParam, LOVE3D_MAX_LIGHTS);
	}

	if (isUniformUsable(activeUniforms.lightColor))
	{
		memcpy(activeUniforms.lightColor->data, lightColorData, sizeof(Vector4) * LOVE3D_MAX_LIGHTS);
		shader->updateUniform(activeUniforms.lightColor, LOVE3D_MAX_LIGHTS);
	}
}

void Graphics3D::drawMesh(Mesh3D *mesh, const Matrix4 &modelMatrix)
{
	if (!in3D)
		throw love::Exception("draw can only be called between begin3D and end3d.");

	if (mesh == nullptr)
		throw love::Exception("Expected a Mesh3D argument.");

	for (float value : modelMatrix.m)
	{
		if (!std::isfinite(value))
			throw love::Exception("3D model matrices must contain only finite values.");
	}

	Vector3 boundsMin, boundsMax;
	float distance = 0.0f;
	if (mesh->getBounds(boundsMin, boundsMax))
	{
		Vector3 worldCenter = modelMatrix * ((boundsMin + boundsMax) * 0.5f);
		Vector3 cameraPosition = camera.get() != nullptr ? camera->getPosition() : Vector3();
		distance = (worldCenter - cameraPosition).length();
	}

	Mesh3D *selectedMesh = mesh->getLOD(distance);
	QueuedDraw queued;
	queued.mesh.set(selectedMesh);
	queued.material.set(selectedMesh->getMaterial());
	queued.modelMatrix = modelMatrix;

	drawQueue.push_back(queued);
	stats.drawsPassed++;
}

void Graphics3D::drawInstanced(Mesh3D *mesh, const std::vector<Matrix4> &transforms)
{
	if (!in3D)
		throw love::Exception("drawInstanced can only be called between begin3D and end3d.");
	if (mesh == nullptr)
		throw love::Exception("Expected a Mesh3D argument.");

	for (const Matrix4 &transform : transforms)
	{
		for (float value : transform.m)
		{
			if (!std::isfinite(value))
				throw love::Exception("3D instance matrices must contain only finite values.");
		}
	}

	for (const Matrix4 &transform : transforms)
	{
		drawMesh(mesh, transform);
		drawQueue.back().explicitInstanced = true;
	}
}

void Graphics3D::drawModel(Model3D *model, const Matrix4 &modelMatrix)
{
	if (!in3D)
		throw love::Exception("draw can only be called between begin3D and end3d.");

	if (model == nullptr)
		throw love::Exception("Expected a Model3D argument.");

	// Queue each part as its own draw with the composed local matrix.
	for (size_t part = 0; part < model->getPartCount(); part++)
	{
		Mesh3D *partMesh = model->getPartMesh(part);

		if (partMesh == nullptr)
			continue;

		Matrix4 partMatrix = modelMatrix * model->getPartMatrix(part);
		Vector3 boundsMin, boundsMax;
		float distance = 0.0f;
		if (partMesh->getBounds(boundsMin, boundsMax))
		{
			Vector3 worldCenter = partMatrix * ((boundsMin + boundsMax) * 0.5f);
			Vector3 cameraPosition = camera.get() != nullptr ? camera->getPosition() : Vector3();
			distance = (worldCenter - cameraPosition).length();
		}
		Mesh3D *selectedMesh = partMesh->getLOD(distance);
		QueuedDraw queued;
		queued.mesh.set(selectedMesh);
		queued.material.set(model->getPartMaterial(part) != nullptr
			? model->getPartMaterial(part) : selectedMesh->getMaterial());
		queued.modelMatrix = partMatrix;

		for (float value : queued.modelMatrix.m)
		{
			if (!std::isfinite(value))
				throw love::Exception("3D model matrices must contain only finite values.");
		}

		drawQueue.push_back(queued);
		stats.drawsPassed++;
	}
}

void Graphics3D::drawNode(Node3D *node)
{
	if (!in3D)
		throw love::Exception("draw can only be called between begin3D and end3d.");
	if (node == nullptr)
		throw love::Exception("Expected a Node3D argument.");

	std::function<void(Node3D *)> queueSubtree = [&](Node3D *current)
	{
		Matrix4 world = current->getWorldMatrix();
		if (current->getMesh() != nullptr)
			drawMesh(current->getMesh(), world);
		else if (current->getModel() != nullptr)
			drawModel(current->getModel(), world);

		for (size_t i = 0; i < current->getChildCount(); i++)
			queueSubtree(current->getChild(i));
	};
	queueSubtree(node);
}

void Graphics3D::debugLine(const Vector3 &from, const Vector3 &to, const Vector4 &color, bool depthTest)
{
	if (!in3D)
		throw love::Exception("debugLine can only be called between begin3D and end3d.");
	if (!std::isfinite(color.x) || !std::isfinite(color.y) || !std::isfinite(color.z) || !std::isfinite(color.w))
		throw love::Exception("Debug line color values must be finite.");

	Vector3 delta = to - from;
	float length = delta.length();
	if (!std::isfinite(length))
		throw love::Exception("Debug line endpoints must be finite.");
	if (length <= 1e-6f)
		return;

	if (debugBoxMesh.get() == nullptr)
		debugBoxMesh.set(newBox(1.0f, 1.0f, 1.0f), Acquire::NORETAIN);

	Vector3 direction = delta / length;
	Vector3 up = fabsf(direction.y) < 0.99f ? Vector3(0.0f, 1.0f, 0.0f) : Vector3(1.0f, 0.0f, 0.0f);
	Quaternion rotation = Quaternion::lookRotation(-direction, up);
	Matrix4 modelMatrix;
	modelMatrix.setTransformQ((from + to) * 0.5f, rotation, Vector3(0.015f, 0.015f, length));

	StrongRef<Material3D> material(new Material3D(), Acquire::NORETAIN);
	material->setColor(color);
	material->setUnlit(true);
	material->setCullMode(love::graphics::CULL_NONE);

	QueuedDraw queued;
	queued.mesh.set(debugBoxMesh.get());
	queued.material.set(material.get());
	queued.modelMatrix = modelMatrix;
	queued.debug = true;
	queued.depthTest = depthTest;
	drawQueue.push_back(queued);
	stats.drawsPassed++;
}

void Graphics3D::debugBox(const Vector3 &boundsMin, const Vector3 &boundsMax, const Vector4 &color, bool depthTest)
{
	if (!in3D)
		throw love::Exception("debugBox can only be called between begin3D and end3d.");
	if (!std::isfinite(boundsMin.x) || !std::isfinite(boundsMin.y) || !std::isfinite(boundsMin.z)
		|| !std::isfinite(boundsMax.x) || !std::isfinite(boundsMax.y) || !std::isfinite(boundsMax.z))
		throw love::Exception("Debug box bounds must be finite.");
	if (boundsMin.x > boundsMax.x || boundsMin.y > boundsMax.y || boundsMin.z > boundsMax.z)
		throw love::Exception("Debug box minimum bounds must not exceed maximum bounds.");

	Vector3 corners[8];
	for (int i = 0; i < 8; i++)
	{
		corners[i] = Vector3((i & 1) ? boundsMax.x : boundsMin.x,
			(i & 2) ? boundsMax.y : boundsMin.y,
			(i & 4) ? boundsMax.z : boundsMin.z);
	}

	static const int edges[12][2] = {
		{ 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 },
		{ 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
	};

	for (const auto &edge : edges)
		debugLine(corners[edge[0]], corners[edge[1]], color, depthTest);
}

void Graphics3D::debugSphere(const Vector3 &center, float radius, const Vector4 &color, bool depthTest)
{
	if (!in3D)
		throw love::Exception("debugSphere can only be called between begin3D and end3d.");
	if (!std::isfinite(radius) || radius <= 0.0f)
		throw love::Exception("Debug sphere radius must be a positive finite value.");
	if (!std::isfinite(center.x) || !std::isfinite(center.y) || !std::isfinite(center.z)
		|| !std::isfinite(color.x) || !std::isfinite(color.y) || !std::isfinite(color.z) || !std::isfinite(color.w))
		throw love::Exception("Debug sphere center and color values must be finite.");

	const int segments = 16;
	const float tau = 6.28318530718f;
	for (int plane = 0; plane < 3; plane++)
	{
		for (int segment = 0; segment < segments; segment++)
		{
			float a = tau * segment / segments;
			float b = tau * (segment + 1) / segments;
			Vector3 from, to;

			if (plane == 0)
			{
				from = center + Vector3(cosf(a), sinf(a), 0.0f) * radius;
				to = center + Vector3(cosf(b), sinf(b), 0.0f) * radius;
			}
			else if (plane == 1)
			{
				from = center + Vector3(cosf(a), 0.0f, sinf(a)) * radius;
				to = center + Vector3(cosf(b), 0.0f, sinf(b)) * radius;
			}
			else
			{
				from = center + Vector3(0.0f, cosf(a), sinf(a)) * radius;
				to = center + Vector3(0.0f, cosf(b), sinf(b)) * radius;
			}

			debugLine(from, to, color, depthTest);
		}
	}
}

void Graphics3D::debugNormals(Mesh3D *mesh, const Matrix4 &modelMatrix, float length, bool depthTest)
{
	if (!in3D)
		throw love::Exception("debugNormals can only be called between begin3D and end3d.");
	if (mesh == nullptr)
		throw love::Exception("Expected a Mesh3D argument for debugNormals.");
	if (!std::isfinite(length) || length <= 0.0f)
		throw love::Exception("Debug normal length must be a positive finite value.");

	const std::vector<Vertex3D> &vertices = mesh->getVertexData();
	if (vertices.empty())
		return;

	Matrix4 normalMatrix = modelMatrix.inverse().transposed();
	const size_t maxLines = 2048;
	const size_t stride = std::max((size_t) 1, (vertices.size() + maxLines - 1) / maxLines);
	const Vector4 color(0.25f, 1.0f, 0.35f, 1.0f);

	for (size_t i = 0; i < vertices.size(); i += stride)
	{
		const Vertex3D &vertex = vertices[i];
		Vector3 position(vertex.position[0], vertex.position[1], vertex.position[2]);
		Vector3 normal(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
		Vector3 worldPosition = modelMatrix * position;
		Vector4 transformedNormal = normalMatrix * Vector4(normal, 0.0f);
		Vector3 worldNormal(transformedNormal.x, transformedNormal.y, transformedNormal.z);
		worldNormal.normalize();
		if (worldNormal.length() > 0.0f)
			debugLine(worldPosition, worldPosition + worldNormal * length, color, depthTest);
	}
}

void Graphics3D::end3D()
{
	if (!in3D)
		throw love::Exception("end3d can only be called after begin3D.");

	love::graphics::Graphics *gfx = getGraphics();

	try
	{
		flushQueuedDraws();
	}
	catch (...)
	{
		drawQueue.clear();
		in3D = false;
		gfx->pop();
		throw;
	}

	in3D = false;
	gfx->pop();
}

void Graphics3D::clearDepth()
{
	if (!in3D)
		throw love::Exception("clearDepth can only be called between begin3D and end3d.");

	love::graphics::Graphics *gfx = getGraphics();
	gfx->clear(love::graphics::OptionalColorD(), OptionalInt(), OptionalDouble(1.0));
}

static bool isOutsideFrustum(const Matrix4 &viewProjection, const Matrix4 &modelMatrix,
	const Vector3 &boundsMin, const Vector3 &boundsMax)
{
	bool outside[6] = { true, true, true, true, true, true };

	for (int corner = 0; corner < 8; corner++)
	{
		Vector3 local(
			(corner & 1) ? boundsMax.x : boundsMin.x,
			(corner & 2) ? boundsMax.y : boundsMin.y,
			(corner & 4) ? boundsMax.z : boundsMin.z);
		Vector4 world = modelMatrix * Vector4(local, 1.0f);
		Vector4 clip = viewProjection * world;
		float planeValues[6] = {
			clip.x + clip.w,
			clip.w - clip.x,
			clip.y + clip.w,
			clip.w - clip.y,
			clip.z + clip.w,
			clip.w - clip.z,
		};

		for (int plane = 0; plane < 6; plane++)
		{
			if (planeValues[plane] >= 0.0f)
				outside[plane] = false;
		}
	}

	for (bool isOutside : outside)
	{
		if (isOutside)
			return true;
	}

	return false;
}

void Graphics3D::flushQueuedDraws()
{
	if (drawQueue.empty())
		return;

	love::graphics::Graphics *gfx = getGraphics();

	love::graphics::Shader *shader = litShader.get();

	struct SortedDraw
	{
		const QueuedDraw *queued;
		Material3D *material;
		float cameraDepth;
		bool transparent;
		bool additive;
	};

	std::vector<SortedDraw> sortedDraws;
	sortedDraws.reserve(drawQueue.size());

	Matrix4 viewMatrix = camera.get() != nullptr ? camera->getViewMatrix() : Matrix4::identity();
	Matrix4 projectionMatrix = camera.get() != nullptr ? camera->getProjectionMatrix()
		: Matrix4::perspective(70.0f * 3.14159265359f / 180.0f, 1.0f, 0.1f, 1000.0f);
	Matrix4 viewProjection = projectionMatrix * viewMatrix;

	for (const QueuedDraw &queued : drawQueue)
	{
		Mesh3D *mesh = queued.mesh.get();
		if (mesh == nullptr)
			continue;

		Material3D *material = queued.material.get();
		if (material == nullptr)
			material = mesh->getMaterial();

		Vector3 boundsMin(0.0f, 0.0f, 0.0f), boundsMax(0.0f, 0.0f, 0.0f);
		bool hasBounds = mesh->getBoundingBox(boundsMin, boundsMax);
		if (hasBounds && isOutsideFrustum(viewProjection, queued.modelMatrix, boundsMin, boundsMax))
		{
			stats.culled++;
			continue;
		}

		Vector3 localCenter = (boundsMin + boundsMax) * 0.5f;

		Vector3 worldCenter = queued.modelMatrix * localCenter;
		float cameraDepth = -(viewMatrix * worldCenter).z;
		if (!std::isfinite(cameraDepth))
			throw love::Exception("3D draw depth must be finite.");

		sortedDraws.push_back({
			&queued,
			material,
			cameraDepth,
			(material != nullptr && material->isTransparent()) || mesh->hasTransparentVertex(),
			material != nullptr && material->isAdditiveBlend()
		});
	}

	std::stable_sort(sortedDraws.begin(), sortedDraws.end(), [](const SortedDraw &a, const SortedDraw &b)
	{
		if (a.queued->debug != b.queued->debug)
			return !a.queued->debug;
		if (a.queued->debug && b.queued->debug && a.queued->depthTest != b.queued->depthTest)
			return a.queued->depthTest;

		if (a.transparent != b.transparent)
			return !a.transparent;

		if (a.transparent)
			return a.cameraDepth > b.cameraDepth;

		std::less<Mesh3D *> meshLess;
		if (a.queued->mesh.get() != b.queued->mesh.get())
			return meshLess(a.queued->mesh.get(), b.queued->mesh.get());
		if (a.material != b.material)
			return std::less<Material3D *>()(a.material, b.material);
		return a.queued->explicitInstanced && !b.queued->explicitInstanced;
	});

	// The 3D shader must be current for our uniform updates and draws.
	// (2D draws issued during the pass would have switched it.)
	gfx->setShader(shader);
	gfx->setBlendMode(love::graphics::BLEND_REPLACE, love::graphics::BLENDALPHA_MULTIPLY);
	gfx->setDepthMode(love::graphics::COMPARE_LEQUAL, true);

	bool transparentPass = false;
	bool additivePass = false;
	bool debugPass = false;
	bool debugDepthTest = true;
	bool debugTransparent = false;
	const bool supportsInstancing = gfx->getCapabilities().features[love::graphics::Graphics::FEATURE_INSTANCING];
	for (size_t index = 0; index < sortedDraws.size();)
	{
		const SortedDraw &entry = sortedDraws[index];
		if (entry.queued->debug && !debugPass)
		{
			gfx->setBlendMode(love::graphics::BLEND_REPLACE, love::graphics::BLENDALPHA_MULTIPLY);
			gfx->setDepthMode(love::graphics::COMPARE_LEQUAL, true);
			transparentPass = false;
			debugPass = true;
			debugDepthTest = true;
			debugTransparent = false;
		}
		if (entry.queued->debug && (entry.queued->depthTest != debugDepthTest || entry.transparent != debugTransparent))
		{
			gfx->setBlendMode(entry.transparent ? love::graphics::BLEND_ALPHA : love::graphics::BLEND_REPLACE,
				love::graphics::BLENDALPHA_MULTIPLY);
			gfx->setDepthMode(entry.queued->depthTest ? love::graphics::COMPARE_LEQUAL : love::graphics::COMPARE_ALWAYS,
				entry.queued->depthTest && !entry.transparent);
			debugDepthTest = entry.queued->depthTest;
			debugTransparent = entry.transparent;
		}
		if (!entry.queued->debug && entry.transparent && (!transparentPass || entry.additive != additivePass))
		{
			gfx->setBlendMode(entry.additive ? love::graphics::BLEND_ADD : love::graphics::BLEND_ALPHA,
				love::graphics::BLENDALPHA_MULTIPLY);
			gfx->setDepthMode(love::graphics::COMPARE_LEQUAL, false);
			transparentPass = true;
			additivePass = entry.additive;
		}

		size_t batchEnd = index + 1;
		bool hasInstancedShader = entry.material == nullptr || entry.material->getShader() == nullptr
			|| entry.material->getInstancedShader() != nullptr;
		bool canBatch = autoBatching || entry.queued->explicitInstanced;
		if (supportsInstancing && canBatch && !entry.queued->debug && !entry.transparent
			&& hasInstancedShader)
		{
			while (batchEnd < sortedDraws.size())
			{
				const SortedDraw &next = sortedDraws[batchEnd];
				if (next.queued->debug || next.transparent
					|| next.queued->mesh.get() != entry.queued->mesh.get()
					|| next.material != entry.material
					|| next.queued->explicitInstanced != entry.queued->explicitInstanced)
					break;
				batchEnd++;
			}
		}

		size_t batchSize = batchEnd - index;
		if (supportsInstancing && canBatch && batchSize > 1 && !entry.queued->debug && !entry.transparent
			&& hasInstancedShader)
		{
			std::vector<Matrix4> transforms;
			transforms.reserve(batchSize);
			for (size_t i = index; i < batchEnd; i++)
				transforms.push_back(sortedDraws[i].queued->modelMatrix);

			gfx->setShader(instancedLitShader.get());
			updateSceneUniforms(instancedLitShader.get(), instancedUniforms);
			drawObject(entry.queued->mesh.get(), Matrix4::identity(), entry.material, &transforms);
			stats.drawsFlushed++;
			stats.drawCalls++;
			stats.triangles += (int) (entry.queued->mesh->getIndexCount() / 3) * (int) batchSize;
			index = batchEnd;
			continue;
		}

		gfx->setShader(litShader.get());
		const QueuedDraw &queued = *entry.queued;
		drawObject(queued.mesh.get(), queued.modelMatrix, entry.material);
		stats.drawsFlushed++;
		stats.drawCalls++;
		stats.triangles += (int) (queued.mesh->getIndexCount() / 3);
		index++;
	}

	stats.queued = (int) sortedDraws.size();
	drawQueue.clear();
}

void Graphics3D::drawObject(Mesh3D *mesh, const Matrix4 &modelMatrix, Material3D *material,
	const std::vector<Matrix4> *instances)
{
	love::graphics::Graphics *gfx = getGraphics();

	if (material == nullptr)
		material = mesh->getMaterial();

	if (material == nullptr)
	{
		// Draw with a persistent default material if none was set.
		if (fallbackMaterial.get() == nullptr)
			fallbackMaterial.set(new Material3D(), Acquire::NORETAIN);

		material = fallbackMaterial.get();
	}

	bool hasCustomShader = material->getShader() != nullptr;
	love::graphics::Shader *shader = instances != nullptr
		? hasCustomShader ? material->getInstancedShader() : instancedLitShader.get()
		: hasCustomShader ? material->getShader() : litShader.get();
	UniformInfos customUniforms;
	const UniformInfos *uniformInfo = hasCustomShader ? &customUniforms
		: instances != nullptr ? &instancedUniforms : &uniforms;
	if (hasCustomShader)
	{
		cacheUniformInfos(shader, customUniforms);
		uniformInfo = &customUniforms;
		gfx->setShader(shader);
		updateSceneUniforms(shader, *uniformInfo);
	}
	else if (instances != nullptr)
		gfx->setShader(shader);
	const UniformInfos &activeUniforms = *uniformInfo;

	// Per-object uniforms.
	if (instances == nullptr && isUniformUsable(activeUniforms.modelMatrix))
	{
		memcpy(activeUniforms.modelMatrix->data, modelMatrix.m, sizeof(float) * 16);
		shader->updateUniform(activeUniforms.modelMatrix, 1);
	}

	if (instances == nullptr && isUniformUsable(activeUniforms.normalMatrix))
	{
		// Inverse-transpose of the model's rotation part, embedded into a
		// mat4 for the shader.
		Matrix3 normalMatrix = modelMatrix.normalMatrix();

		Matrix4 packed = Matrix4::identity();
		for (int c = 0; c < 3; c++)
		{
			for (int r = 0; r < 3; r++)
				packed(c, r) = normalMatrix(c, r);
		}

		memcpy(activeUniforms.normalMatrix->data, packed.m, sizeof(float) * 16);
		shader->updateUniform(activeUniforms.normalMatrix, 1);
	}

	Material3D::UniformValues values = material->getUniformValues();

	if (isUniformUsable(activeUniforms.materialColor))
	{
		float data[4] = { values.materialColor.r, values.materialColor.g, values.materialColor.b, values.materialColor.a };
		memcpy(activeUniforms.materialColor->data, data, sizeof(float) * 4);
		shader->updateUniform(activeUniforms.materialColor, 1);
	}

	if (isUniformUsable(activeUniforms.materialSpec))
	{
		float hasNormalMap = material->getNormalTexture() != nullptr ? 1.0f : 0.0f;
		float data[4] = { values.specGloss.r, values.specGloss.g, values.specGloss.b, hasNormalMap };
		memcpy(activeUniforms.materialSpec->data, data, sizeof(float) * 4);
		shader->updateUniform(activeUniforms.materialSpec, 1);
	}

	if (isUniformUsable(activeUniforms.materialEmissive))
	{
		float data[4] = { values.emissiveColor.r, values.emissiveColor.g, values.emissiveColor.b, 0.0f };
		memcpy(activeUniforms.materialEmissive->data, data, sizeof(float) * 4);
		shader->updateUniform(activeUniforms.materialEmissive, 1);
	}

	if (isUniformUsable(activeUniforms.materialAlpha))
	{
		float data[2] = { material->getAlphaCutoff(), material->isAlphaMask() ? 1.0f : 0.0f };
		memcpy(activeUniforms.materialAlpha->data, data, sizeof(data));
		shader->updateUniform(activeUniforms.materialAlpha, 1);
	}

	// Material textures: only bind ones that are actually set; the Shader
	// constructor default-binds white textures for the rest.
	love::graphics::Texture *diffuseTex = material->getDiffuseTexture();

	if (isUniformUsable(activeUniforms.diffuseTexture) && diffuseTex != nullptr)
	{
		love::graphics::Texture *textures[1] = { diffuseTex };
		shader->sendTextures(activeUniforms.diffuseTexture, textures, 1);
	}

	if (isUniformUsable(activeUniforms.normalTexture) && material->getNormalTexture() != nullptr)
	{
		love::graphics::Texture *textures[1] = { material->getNormalTexture() };
		shader->sendTextures(activeUniforms.normalTexture, textures, 1);
	}

	if (isUniformUsable(activeUniforms.specularTexture) && material->getSpecularTexture() != nullptr)
	{
		love::graphics::Texture *textures[1] = { material->getSpecularTexture() };
		shader->sendTextures(activeUniforms.specularTexture, textures, 1);
	}

	if (isUniformUsable(activeUniforms.emissiveTexture) && material->getEmissiveTexture() != nullptr)
	{
		love::graphics::Texture *textures[1] = { material->getEmissiveTexture() };
		shader->sendTextures(activeUniforms.emissiveTexture, textures, 1);
	}

	if (instances != nullptr)
		mesh->drawInstanced(gfx, *instances, material->getCullMode(), diffuseTex);
	else
		mesh->drawBatched(gfx, material->getCullMode(), diffuseTex);
}

} // graphics3d
} // love
