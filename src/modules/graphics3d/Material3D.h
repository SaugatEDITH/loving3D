/**
 * LOVING3D - surface materials for the LOVE game engine.
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
 * Describes how a 3D surface is shaded. This initial implementation supports
 * a Blinn-Phong lighting model (the material is an "extensible interface"
 * used by the scene shaders). A PBR upgrade path is planned.
 *
 * Values map to uniforms on the 3D shaders. Textures are love.graphics
 * Image objects on the Lua side. Note that the Phong shaders live in
 * Graphics3D.cpp (scene shader sources).
 *
 * Base and emissive RGB values are normalized display-space (sRGB-like)
 * values; the standard shader converts them to linear light. Alpha is
 * linear opacity. Texture and vertex RGB values are multiplied as sampled.
 **/

#pragma once

#include "Vector3.h"

#include "modules/graphics/Texture.h"
#include "modules/graphics/Shader.h"
#include "common/Object.h"
#include "common/Color.h"

#include <vector>
#include <string>

namespace love
{
namespace graphics3d
{

class Material3D : public love::Object
{
public:
	enum AlphaMode
	{
		ALPHA_AUTO,
		ALPHA_OPAQUE,
		ALPHA_MASK,
		ALPHA_BLEND,
	};

	static love::Type type;

	Material3D();
	virtual ~Material3D();

	void setColor(float r, float g, float b, float a);
	void setColor(const Vector4 &c);
	Vector4 getColor() const;

	void setDiffuseTexture(love::graphics::Texture *texture);
	love::graphics::Texture *getDiffuseTexture() const { return diffuseTexture.get(); }

	void setNormalTexture(love::graphics::Texture *texture);
	love::graphics::Texture *getNormalTexture() const { return normalTexture.get(); }

	void setSpecularTexture(love::graphics::Texture *texture);
	love::graphics::Texture *getSpecularTexture() const { return specularTexture.get(); }

	void setEmissiveTexture(love::graphics::Texture *texture);
	love::graphics::Texture *getEmissiveTexture() const { return emissiveTexture.get(); }
	void setMetallicRoughnessTexture(love::graphics::Texture *texture);
	love::graphics::Texture *getMetallicRoughnessTexture() const { return metallicRoughnessTexture.get(); }
	void setOcclusionTexture(love::graphics::Texture *texture);
	love::graphics::Texture *getOcclusionTexture() const { return occlusionTexture.get(); }
	void setOcclusionStrength(float value);
	float getOcclusionStrength() const { return occlusionStrength; }
	void setMetallic(float value);
	float getMetallic() const { return metallic; }
	void setRoughness(float value);
	float getRoughness() const { return roughness; }
	void setPBR(bool enabled) { pbr = enabled; }
	bool isPBR() const { return pbr; }
	void setShader(love::graphics::Shader *shader) { customShader.set(shader); }
	love::graphics::Shader *getShader() const { return customShader.get(); }
	void setInstancedShader(love::graphics::Shader *shader) { instancedCustomShader.set(shader); }
	love::graphics::Shader *getInstancedShader() const { return instancedCustomShader.get(); }
	void setFragmentFunction(const std::string &source) { fragmentFunction = source; }
	const std::string &getFragmentFunction() const { return fragmentFunction; }

	// Specular "shiny" contribution in 0..1 and exponent (16..64+).
	void setSpecularParams(float specularStrength, float shininessExponent);
	void getSpecularParams(float &specularStrength, float &shininessExponent) const;

	void setEmissiveColor(const Vector3 &color);
	Vector3 getEmissiveColor() const;

	// Material opacity affects albedo alpha in shader mode.
	void setOpacity(float o);
	float getOpacity() const { return color.a; }

	// Enables alpha blending for diffuse textures with transparency.
	// Material opacity below 1.0 also enables the transparent pass.
	void setTransparent(bool transparent);
	bool isTransparent() const
	{
		if (alphaMode == ALPHA_OPAQUE || alphaMode == ALPHA_MASK)
			return false;
		return alphaMode == ALPHA_BLEND || transparent || color.a < 1.0f || additiveBlend;
	}
	void setAlphaMode(AlphaMode mode) { alphaMode = mode; }
	void setAlphaCutoff(float cutoff);
	float getAlphaCutoff() const { return alphaCutoff; }
	bool isAlphaMask() const { return alphaMode == ALPHA_MASK; }
	void setAdditiveBlend(bool additive) { additiveBlend = additive; }
	bool isAdditiveBlend() const { return additiveBlend; }

	// Unlit surfaces ignore the scene lighting; only diffuse texel and tint.
	void setUnlit(bool unlit);
	bool isUnlit() const { return unlit; }

	// Two-sided rendering skips backface culling.
	void setCullMode(love::graphics::CullMode mode);
	love::graphics::CullMode getCullMode() const { return cullMode; }

	/**
	 * "Uniform layout" description — used by the standard shaders:
	 *   materialColor  vec4(diffuse * opacity + emissive tint)
	 *   specGloss      vec4(specularStrength, shininess, unlitFlag, cullMode)
	 *   emissiveColor  vec4(emissive rgb, 0)
	 **/
	struct UniformValues
	{
		Colorf materialColor;
		Colorf specGloss;
		Colorf emissiveColor;
	};

	UniformValues getUniformValues() const;

private:

	Colorf color;    // diffuse color * opacity.
	Colorf specGloss; // specular + glossiness (see shader doc).
	Colorf emissive;  // emissive color set beyond texture.

	StrongRef<love::graphics::Texture> diffuseTexture;
	StrongRef<love::graphics::Texture> normalTexture;
	StrongRef<love::graphics::Texture> specularTexture;
	StrongRef<love::graphics::Texture> emissiveTexture;
	StrongRef<love::graphics::Texture> metallicRoughnessTexture;
	StrongRef<love::graphics::Texture> occlusionTexture;
	StrongRef<love::graphics::Shader> customShader;
	StrongRef<love::graphics::Shader> instancedCustomShader;
	std::string fragmentFunction;

	bool transparent = false;
	bool additiveBlend = false;
	AlphaMode alphaMode = ALPHA_AUTO;
	float alphaCutoff = 0.5f;
	bool unlit = false;
	bool pbr = false;
	float metallic = 0.0f;
	float roughness = 0.5f;
	float occlusionStrength = 1.0f;
	love::graphics::CullMode cullMode = love::graphics::CULL_BACK;

}; // Material3D

} // graphics3d
} // love
