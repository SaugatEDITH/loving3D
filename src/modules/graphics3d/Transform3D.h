/**
 * LOVING3D - 3D transforms for the LOVE game engine.
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
 * A 3D transform: position, rotation (quaternion) and scale. Used to place
 * rendered 3D objects in world space, and as the future basis for scene
 * graph hierarchy support.
 **/

#pragma once

#include "Quaternion.h"
#include "Matrix4.h"

namespace love
{
namespace graphics3d
{

class Transform3D
{
public:

	Transform3D()
		: scalev(1.0f, 1.0f, 1.0f)
		, dirty(true)
	{}

	void setPosition(const Vector3 &p) { positionv = p; dirty = true; }
	void setPosition(float x, float y, float z) { setPosition(Vector3(x, y, z)); }
	void translate(const Vector3 &delta) { setPosition(positionv + delta); }

	void setRotationQuaternion(const Quaternion &q) { rotationv = q; dirty = true; }
	void setRotationEulerDegrees(const Vector3 &degrees)
	{
		rotationv = Quaternion::fromEuler(degrees.x * 3.14159265359f / 180.0f, degrees.y * 3.14159265359f / 180.0f, degrees.z * 3.14159265359f / 180.0f);
		dirty = true;
	}

	// Rotate around the given world-space axis.
	void rotate(float degrees, const Vector3 &axis)
	{
		Quaternion q = Quaternion::fromAxisAngle(axis, degrees * 3.14159265359f / 180.0f);
		rotationv = q * rotationv; // world-space: premultiply.
		dirty = true;
	}

	void setScale(const Vector3 &s) { scalev = s; dirty = true; }
	void setScale(float uniform) { setScale(Vector3(uniform, uniform, uniform)); }
	void scaleBy(const Vector3 &delta) { setScale(scalev * delta); }

	const Vector3 &getPosition() const { return positionv; }
	const Quaternion &getRotationQuaternion() const { return rotationv; }
	const Vector3 &getScale() const { return scalev; }

	Vector3 getEulerAnglesDegrees() const
	{
		Vector3 rad = rotationv.toEulerAngleVector();
		return Vector3(rad.x * 180.0f / 3.14159265359f, rad.y * 180.0f / 3.14159265359f, rad.z * 180.0f / 3.14159265359f);
	}

	Matrix4 getLocalMatrix() const
	{
		if (dirty)
		{
			// Row composition order: translation applied last (T * R * S).
			const_cast<Transform3D *>(this)->cachedLocal.setTransformQ(positionv, rotationv, scalev);
			const_cast<Transform3D *>(this)->dirty = false;
		}
		return cachedLocal;
	}

	/**
	 * Sets this transform's world matrix to "parentLocal * local". For
	 * scene graph use.
	 **/
	Matrix4 getWorldMatrix(const Matrix4 &parentWorld) const
	{
		return parentWorld * getLocalMatrix();
	}

private:

	Vector3 positionv;
	Quaternion rotationv;
	Vector3 scalev;

	Matrix4 cachedLocal;
	bool dirty = true;

}; // Transform3D

} // graphics3d
} // love
