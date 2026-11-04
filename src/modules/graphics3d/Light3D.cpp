/**
 * LOVING3D - 3D light implementation.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org). The underlying engine
 * remains freely distributable under the terms of its original zlib license
 * (see license.txt).
 **/

#include "Light3D.h"

#include "common/Exception.h"

#include <cmath>

namespace love
{
namespace graphics3d
{

love::Type Light3D::type("Light3D", &Object::type);

Light3D::Light3D(LightType lightType)
	: type_(lightType)
	, direction(0.0f, -1.0f, 0.0f)
	, color(1.0f, 1.0f, 1.0f)
{
}

Light3D::~Light3D()
{
}

void Light3D::setPosition(float x, float y, float z)
{
	position = Vector3(x, y, z);
}

void Light3D::setPosition(const Vector3 &p)
{
	setPosition(p.x, p.y, p.z);
}

void Light3D::setDirection(float x, float y, float z)
{
	Vector3 d(x, y, z);
	float len = d.length();
	if (len <= 0.0f)
		throw love::Exception("Light direction must be a non-zero vector.");

	direction = d / len;
}

void Light3D::setDirection(const Vector3 &d)
{
	setDirection(d.x, d.y, d.z);
}

void Light3D::setColor(float r, float g, float b)
{
	color = Vector3(r, g, b);
}

void Light3D::setColor(const Vector3 &c)
{
	setColor(c.x, c.y, c.z);
}

void Light3D::setIntensity(float i)
{
	if (i < 0.0f)
		throw love::Exception("Light intensity cannot be negative.");

	intensity = i;
}

void Light3D::setRange(float r)
{
	if (r <= 0.0f)
		throw love::Exception("Light range must be greater than zero.");

	range = r;
}

void Light3D::setSpotAngles(float degreesInner, float degreesOuter)
{
	if (degreesInner < 0.0f || degreesOuter <= degreesInner || degreesOuter >= 90.0f)
		throw love::Exception("Spot angles must satisfy 0 <= inner < outer < 90 degrees.");

	spotInnerDegrees = degreesInner;
	spotOuterDegrees = degreesOuter;
}

void Light3D::setFalloff(float f)
{
	if (f < 0.0f)
		throw love::Exception("Light falloff cannot be negative.");

	falloff = f;
}

Vector3 Light3D::getNormalizedDirection() const
{
	float len = direction.length();
	if (len <= 0.0f)
		return Vector3(0.0f, -1.0f, 0.0f);

	return direction / len;
}

} // graphics3d
} // love
