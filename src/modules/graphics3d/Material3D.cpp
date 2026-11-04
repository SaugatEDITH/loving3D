/**
 * LOVING3D - surface material implementation.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org). The underlying engine
 * remains freely distributable under the terms of its original zlib license
 * (see license.txt).
 **/

#include "Material3D.h"

#include "common/Exception.h"

#include <cmath>

namespace love
{
namespace graphics3d
{

love::Type Material3D::type("Material3D", &Object::type);

Material3D::Material3D()
	: color(1.0f, 1.0f, 1.0f, 1.0f)
	, specGloss(0.2f, 32.0f, 0.0f, 0.0f)
	, emissive(0.0f, 0.0f, 0.0f, 0.0f)
{
}

Material3D::~Material3D()
{
}

void Material3D::setColor(float r, float g, float b, float a)
{
	color.r = r;
	color.g = g;
	color.b = b;
	color.a = a;
}

void Material3D::setColor(const Vector4 &c)
{
	setColor(c.x, c.y, c.z, c.w);
}

Vector4 Material3D::getColor() const
{
	return Vector4(color.r, color.g, color.b, color.a);
}

void Material3D::setOpacity(float o)
{
	color.a = o;
}

void Material3D::setTransparent(bool transparent_)
{
	transparent = transparent_;
}

void Material3D::setAlphaCutoff(float cutoff)
{
	if (!std::isfinite(cutoff) || cutoff < 0.0f || cutoff > 1.0f)
		throw love::Exception("Material alpha cutoff must be finite and between 0 and 1.");
	alphaCutoff = cutoff;
}

void Material3D::setDiffuseTexture(love::graphics::Texture *texture)
{
	diffuseTexture.set(texture);
}

void Material3D::setNormalTexture(love::graphics::Texture *texture)
{
	normalTexture.set(texture);
}

void Material3D::setSpecularTexture(love::graphics::Texture *texture)
{
	specularTexture.set(texture);
}

void Material3D::setEmissiveTexture(love::graphics::Texture *texture)
{
	emissiveTexture.set(texture);
}

void Material3D::setSpecularParams(float specularStrength, float shininessExponent)
{
	specGloss.r = specularStrength;
	specGloss.g = shininessExponent;
}

void Material3D::getSpecularParams(float &specularStrength, float &shininessExponent) const
{
	specularStrength = specGloss.r;
	shininessExponent = specGloss.g;
}

void Material3D::setEmissiveColor(const Vector3 &c)
{
	emissive.r = c.x;
	emissive.g = c.y;
	emissive.b = c.z;
}

Vector3 Material3D::getEmissiveColor() const
{
	return Vector3(emissive.r, emissive.g, emissive.b);
}

void Material3D::setUnlit(bool unlit_)
{
	unlit = unlit_;
}

void Material3D::setCullMode(love::graphics::CullMode mode)
{
	cullMode = mode;
}

Material3D::UniformValues Material3D::getUniformValues() const
{
	UniformValues uv;
	uv.materialColor = color;
	uv.specGloss = specGloss;
	uv.specGloss.b = unlit ? 1.0f : 0.0f;
	uv.specGloss.a = (float) cullMode;
	uv.emissiveColor = emissive;
	return uv;
}

} // graphics3d
} // love
