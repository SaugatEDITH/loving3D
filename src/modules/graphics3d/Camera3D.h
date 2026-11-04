/**
 * LOVING3D - 3D cameras for the LOVE game engine.
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
 * A 3D camera, with either a perspective or orthographic projection.
 *
 * The camera stores its pose as position + rotation quaternion, so it can
 * act as the "player eye" for first person games, freely blend toward
 * targets for third person setups, and produce view and projection
 * matrices for the 3D rendering pipeline.
 *
 * Matrices are computed lazily and cached whenever their inputs change.
 * fovY is in degrees, measured from the bottom plane to the top plane around
 * the screen height.
 *
 * NOTE: positions and camera math here assume the engine's right-handed
 * coordinate system: the camera's default forward direction is -Z, world
 * +Y points up (matching Lua APIs like newCamera's target argument).
 **/

#pragma once

#include "Transform3D.h"

#include "common/Object.h"

namespace love
{
namespace graphics3d
{

class Camera3D : public love::Object
{
public:

	static love::Type type;

	enum ProjectionMode
	{
		PROJECTION_PERSPECTIVE,
		PROJECTION_ORTHOGRAPHIC,
		PROJECTION_MAX_ENUM
	};

	Camera3D();
	virtual ~Camera3D();

	void setPosition(float x, float y, float z);
	void setPosition(const Vector3 &p);
	const Vector3 &getPosition() const { return transform.getPosition(); }

	void setRotation(float degreesX, float degreesY, float degreesZ);
	Vector3 getRotationDegrees() const;
	const Quaternion &getRotationQuaternion() const { return transform.getRotationQuaternion(); }

	void lookAt(float x, float y, float z);
	void lookAt(const Vector3 &target);
	void lookAtOriented(const Vector3 &target, const Vector3 &upHint);

	void setFOV(float degreesFovY);
	float getFOV() const { return fovYDegrees; }

	void setNear(float distNear);
	void setFar(float distFar);
	float getNear() const { return nearPlane; }
	float getFar() const { return farPlane; }

	void setAspect(float aspect);
	float getAspect() const { return aspect; }

	void setProjectionMode(ProjectionMode mode) { projectionMode = mode; viewDirty = true; }
	ProjectionMode getProjectionMode() const { return projectionMode; }

	// Orthographic extents in world units. Only used when in orthographic
	// mode.
	void setOrthoExtents(float left, float right, float bottom, float top);
	void getOrthoExtents(float &left, float &right, float &bottom, float &top) const;

	void moveRelative(float forwardDist, float strafeDist, float upDist);
	void rotateYaw(float degrees);
	void rotatePitch(float degrees);
	void rotateRoll(float degrees);

	Vector3 getForward() const { return transform.getRotationQuaternion().forward(); }
	Vector3 getRight() const { return transform.getRotationQuaternion().right(); }
	Vector3 getUp() const { return transform.getRotationQuaternion().up(); }

	Vector3 worldToScreen(const Vector3 &worldPosition, float viewportWidth, float viewportHeight) const;
	Vector3 screenToWorld(float screenX, float screenY, float depth, float viewportWidth, float viewportHeight) const;
	void screenRay(float screenX, float screenY, float viewportWidth, float viewportHeight,
		Vector3 &origin, Vector3 &direction) const;

	const Matrix4 &getViewMatrix() const;
	Matrix4 getProjectionMatrix() const;
	Matrix4 getViewProjectionMatrix() const;

private:

	Transform3D transform;

	float fovYDegrees = 70.0f;
	float aspect = 16.0f / 9.0f;
	float nearPlane = 0.1f;
	float farPlane = 1000.0f;

	float orthoLeft = -10.0f, orthoRight = 10.0f;
	float orthoBottom = -10.0f, orthoTop = 10.0f;

	ProjectionMode projectionMode = PROJECTION_PERSPECTIVE;
	bool hasLookAtOrientation = false;

	mutable Matrix4 viewMatrix;
	mutable bool viewDirty = true;

}; // Camera3D

} // graphics3d
} // love
