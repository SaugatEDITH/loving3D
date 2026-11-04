/**
 * LOVING3D - 3D camera implementation.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org). The underlying engine
 * remains freely distributable under the terms of its original zlib license
 * (see license.txt).
 **/

#include "Camera3D.h"

#include "common/Exception.h"

#include <cmath>

namespace love
{
namespace graphics3d
{

love::Type Camera3D::type("Camera3D", &Object::type);

Camera3D::Camera3D()
{
}

Camera3D::~Camera3D()
{
}

void Camera3D::setPosition(float x, float y, float z)
{
	transform.setPosition(x, y, z);
	viewDirty = true;
}

void Camera3D::setPosition(const Vector3 &p)
{
	setPosition(p.x, p.y, p.z);
}

void Camera3D::setRotation(float degreesX, float degreesY, float degreesZ)
{
	transform.setRotationEulerDegrees(Vector3(degreesX, degreesY, degreesZ));
	viewDirty = true;
}

void Camera3D::lookAt(float x, float y, float z)
{
	lookAt(Vector3(x, y, z));
}

void Camera3D::lookAt(const Vector3 &target)
{
	lookAtOriented(target, Vector3(0.0f, 1.0f, 0.0f));
}

void Camera3D::lookAtOriented(const Vector3 &target, const Vector3 &upHint)
{
	Vector3 direction = target - getPosition();
	if (direction.lengthsq() <= 0.0f)
		return;

	Vector3 forward = direction.normalized();
	Vector3 right = forward.cross(upHint.normalized());
	const Vector3 previousRight = transform.getRotationQuaternion().right();
	Vector3 projectedPreviousRight = previousRight - forward * previousRight.dot(forward);

	if (right.lengthsq() > 1e-6f)
	{
		right.normalize();

		// The up hint has no unique orientation at its poles. Choose the
		// equivalent basis closest to the previous frame to avoid a 180
		// degree roll when crossing one.
		if (hasLookAtOrientation && projectedPreviousRight.lengthsq() > 1e-6f
			&& right.dot(projectedPreviousRight) < 0.0f)
			right = -right;
	}
	else
	{
		right = projectedPreviousRight;

		if (right.lengthsq() <= 1e-6f)
		{
			right = Vector3(-forward.z, 0.0f, forward.x);
			if (right.lengthsq() <= 1e-6f)
				right = Vector3(1.0f, 0.0f, 0.0f);
		}

		right.normalize();
	}

	Vector3 up = right.cross(forward);
	transform.setRotationQuaternion(Quaternion::lookRotation(forward, up));
	hasLookAtOrientation = true;
	viewDirty = true;
}

void Camera3D::setFOV(float degreesFovY)
{
	if (!std::isfinite(degreesFovY) || degreesFovY <= 0.0f || degreesFovY >= 180.0f)
		throw love::Exception("Vertical field of view must be within (0, 180) degrees.");

	fovYDegrees = degreesFovY;
}

void Camera3D::setNear(float distNear)
{
	if (!std::isfinite(distNear) || distNear <= 0.0f || distNear >= farPlane)
		throw love::Exception("Near plane must be positive and less than the far plane.");

	nearPlane = distNear;
}

void Camera3D::setFar(float distFar)
{
	if (!std::isfinite(distFar) || distFar <= nearPlane)
		throw love::Exception("Far plane must be greater than the near plane.");

	farPlane = distFar;
}

void Camera3D::setAspect(float aspect_)
{
	if (!std::isfinite(aspect_) || aspect_ <= 0.0f)
		throw love::Exception("Camera aspect ratio must be a positive finite number.");

	aspect = aspect_;
}

void Camera3D::setOrthoExtents(float left, float right, float bottom, float top)
{
	if (!std::isfinite(left) || !std::isfinite(right) || !std::isfinite(bottom) || !std::isfinite(top)
		|| right <= left || top <= bottom)
		throw love::Exception("Ortho extents must have right > left and top > bottom.");

	orthoLeft = left;
	orthoRight = right;
	orthoBottom = bottom;
	orthoTop = top;
}

void Camera3D::getOrthoExtents(float &left, float &right, float &bottom, float &top) const
{
	left = orthoLeft;
	right = orthoRight;
	bottom = orthoBottom;
	top = orthoTop;
}

void Camera3D::moveRelative(float forwardDist, float strafeDist, float upDist)
{
	const Quaternion &rot = transform.getRotationQuaternion();
	Vector3 delta = rot.right() * strafeDist + rot.up() * upDist + rot.forward() * forwardDist;
	setPosition(getPosition() + delta);
}

void Camera3D::rotateYaw(float degrees)
{
	transform.rotate(degrees, Vector3(0.0f, 1.0f, 0.0f));
	viewDirty = true;
}

void Camera3D::rotatePitch(float degrees)
{
	transform.rotate(degrees, transform.getRotationQuaternion().right());
	viewDirty = true;
}

void Camera3D::rotateRoll(float degrees)
{
	transform.rotate(degrees, transform.getRotationQuaternion().forward());
	viewDirty = true;
}

static void validateViewport(float width, float height)
{
	if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f)
		throw love::Exception("Camera query viewport dimensions must be positive finite values.");
}

Vector3 Camera3D::worldToScreen(const Vector3 &worldPosition, float viewportWidth, float viewportHeight) const
{
	validateViewport(viewportWidth, viewportHeight);

	Vector4 view = getViewMatrix() * Vector4(worldPosition, 1.0f);
	float depth = -view.z;
	Vector4 clip = getProjectionMatrix() * view;
	if (!std::isfinite(clip.w) || fabsf(clip.w) <= 1e-7f)
		throw love::Exception("Cannot project a world position on the camera plane.");

	float ndcX = clip.x / clip.w;
	float ndcY = clip.y / clip.w;
	return Vector3((ndcX + 1.0f) * viewportWidth * 0.5f,
		(1.0f - ndcY) * viewportHeight * 0.5f, depth);
}

Vector3 Camera3D::screenToWorld(float screenX, float screenY, float depth,
	float viewportWidth, float viewportHeight) const
{
	validateViewport(viewportWidth, viewportHeight);
	if (!std::isfinite(screenX) || !std::isfinite(screenY) || !std::isfinite(depth))
		throw love::Exception("Camera query coordinates and depth must be finite.");

	float ndcX = 2.0f * screenX / viewportWidth - 1.0f;
	float ndcY = 1.0f - 2.0f * screenY / viewportHeight;
	Vector3 position = getPosition();
	Vector3 right = getRight();
	Vector3 up = getUp();
	Vector3 forward = getForward();

	if (projectionMode == PROJECTION_PERSPECTIVE)
	{
		float halfHeight = depth * tanf(fovYDegrees * 3.14159265359f / 360.0f);
		return position + forward * depth + right * (ndcX * halfHeight * aspect)
			+ up * (ndcY * halfHeight);
	}

	float width = orthoRight - orthoLeft;
	float height = orthoTop - orthoBottom;
	return position + forward * depth
		+ right * (orthoLeft + (ndcX + 1.0f) * width * 0.5f)
		+ up * (orthoBottom + (ndcY + 1.0f) * height * 0.5f);
}

void Camera3D::screenRay(float screenX, float screenY, float viewportWidth, float viewportHeight,
	Vector3 &origin, Vector3 &direction) const
{
	validateViewport(viewportWidth, viewportHeight);
	if (!std::isfinite(screenX) || !std::isfinite(screenY))
		throw love::Exception("Camera query coordinates must be finite.");

	float ndcX = 2.0f * screenX / viewportWidth - 1.0f;
	float ndcY = 1.0f - 2.0f * screenY / viewportHeight;
	Vector3 position = getPosition();
	Vector3 forward = getForward();
	Vector3 right = getRight();
	Vector3 up = getUp();

	if (projectionMode == PROJECTION_PERSPECTIVE)
	{
		direction = (forward + right * (ndcX * tanf(fovYDegrees * 3.14159265359f / 360.0f) * aspect)
			+ up * (ndcY * tanf(fovYDegrees * 3.14159265359f / 360.0f))).normalized();
		origin = position;
	}
	else
	{
		float width = orthoRight - orthoLeft;
		float height = orthoTop - orthoBottom;
		origin = position
			+ right * (orthoLeft + (ndcX + 1.0f) * width * 0.5f)
			+ up * (orthoBottom + (ndcY + 1.0f) * height * 0.5f);
		direction = forward;
	}
}

const Matrix4 &Camera3D::getViewMatrix() const
{
	if (viewDirty)
	{
		const Matrix4 &camLocal = transform.getLocalMatrix();

		// The view matrix is the inverse of the camera's world transform
		// (which assumes a uniform-scale camera; we take the fast rigid
		// inverse).
		viewMatrix = camLocal.inverseRigid();
		viewDirty = false;
	}

	return viewMatrix;
}

Matrix4 Camera3D::getProjectionMatrix() const
{
	Matrix4 proj;

	if (projectionMode == PROJECTION_PERSPECTIVE)
	{
		float fovYRadians = fovYDegrees * 3.14159265359f / 180.0f;
		proj = Matrix4::perspective(fovYRadians, aspect, nearPlane, farPlane);
	}
	else
	{
		proj = Matrix4::orthographic(orthoLeft, orthoRight, orthoBottom, orthoTop, nearPlane, farPlane);
	}

	return proj;
}

Matrix4 Camera3D::getViewProjectionMatrix() const
{
	return getProjectionMatrix() * getViewMatrix();
}

Vector3 Camera3D::getRotationDegrees() const
{
	return transform.getEulerAnglesDegrees();
}

} // graphics3d
} // love
