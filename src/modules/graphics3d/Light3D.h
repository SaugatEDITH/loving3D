/**
 * LOVING3D - 3D Lights for the LOVE game engine.
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
 * Blinn-Phong lights: directional, point and spot.
 *
 * Directional lights have no position (only a direction); point lights
 * radiate omnidirectionally from a position with a range-based distance
 * attenuation; spot lights add a cone with smooth inner/outer falloff.
 *
 * The Graphics3D pass uploads the scene lights to the shader as uniformly
 * sized arrays of vec4s (see MAX_LIGHTS). RGB setter values are display-space
 * (sRGB-like); the shader converts them to linear light before applying
 * intensity.
 **/

#pragma once

#include "Vector3.h"

#include "common/Object.h"

namespace love
{
namespace graphics3d
{

class Light3D : public love::Object
{
public:

	static love::Type type;

	enum LightType
	{
		LIGHT_DIRECTIONAL,
		LIGHT_POINT,
		LIGHT_SPOT,
		LIGHT_MAX_ENUM
	};

	Light3D(LightType lightType);
	virtual ~Light3D();

	LightType getType() const { return type_; }

	void setPosition(float x, float y, float z);
	void setPosition(const Vector3 &p);
	Vector3 getPosition() const { return position; }

	/**
	 * The direction the light points in. For directional lights this is
	 * the only property affecting shading aside from color/intensity.
	 **/
	void setDirection(float x, float y, float z);
	void setDirection(const Vector3 &d);
	Vector3 getDirection() const { return direction; }

	void setColor(float r, float g, float b);
	void setColor(const Vector3 &c);
	Vector3 getColor() const { return color; }

	// Brightness multiplier applied to the color.
	void setIntensity(float i);
	float getIntensity() const { return intensity; }

	// World-space distance where a point/spot light's attenuation reaches
	// zero. Directional lights ignore this.
	void setRange(float range);
	float getRange() const { return range; }

	/**
	 * Spot cone half-angles in degrees. Light is fully lit inside the inner
	 * cone, fades to zero at the outer cone edge.
	 **/
	void setSpotAngles(float degreesInner, float degreesOuter);
	float getSpotInnerDegrees() const { return spotInnerDegrees; }
	float getSpotOuterDegrees() const { return spotOuterDegrees; }

	/**
	 * Controls how quickly point/spot light intensity falls off from the
	 * light's center toward the reported range. 1 = physical-ish
	 * (1/pow(distance, falloff*2)) style; 0 = no distance falloff.
	 **/
	void setFalloff(float f);
	float getFalloff() const { return falloff; }

	/**
	 * Returns the light direction normalized (world space, pointing from
	 * the light toward the scene).
	 **/
	Vector3 getNormalizedDirection() const;

private:

	LightType type_;

	Vector3 position;        // point/spot only.
	Vector3 direction;       // directional/spot.
	Vector3 color;           // Display-space RGB converted by the standard shader.
	float intensity = 1.0f;
	float range = 10.0f;
	float spotInnerDegrees = 20.0f;
	float spotOuterDegrees = 30.0f;
	float falloff = 1.0f;

}; // Light3D

} // graphics3d
} // love
