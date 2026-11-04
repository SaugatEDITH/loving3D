/**
 * LOVING3D - quaternion math for the LOVE game engine.
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
 * Quaternions for 3D rotation representation. Positive w represents the
 * shortest rotation path in the same right-handed convention as the rest of
 * graphics3d. For a rotation of angle θ about axis n (unit): q = (n sin(θ/2),
 * cos(θ/2)).
 **/

#pragma once

#include "Vector3.h"
#include "Matrix4.h"
#include <cmath>

namespace love
{
namespace graphics3d
{

class Quaternion
{
public:

	/**
	 * w is the "scalar" part; x, y, z are the "vector" parts.
	 **/
	float x, y, z, w;

	inline Quaternion()
		: x(0), y(0), z(0), w(1)
	{}

	inline Quaternion(float nx, float ny, float nz, float nw)
		: x(nx), y(ny), z(nz), w(nw)
	{}

	/**
	 * Creates a rotation quaternion from an axis-angle pair.
	 **/
	static Quaternion fromAxisAngle(const Vector3 &axis, float radians)
	{
		float halfangle = radians * 0.5f;
		float s = sinf(halfangle);
		return Quaternion(axis.x * s, axis.y * s, axis.z * s, cosf(halfangle));
	}

	/**
	 * Creates a rotation quaternion from intrinsic Euler angles in radians,
	 * applied in Z, then X, then Y order (matching love's 2D yAngle rolling
	 * in the vec3 euler conversions).
	 **/
	static Quaternion fromEuler(float radiansX, float radiansY, float radiansZ)
	{
		float cx = cosf(radiansX * 0.5f);
		float sx = sinf(radiansX * 0.5f);
		float cy = cosf(radiansY * 0.5f);
		float sy = sinf(radiansY * 0.5f);
		float cz = cosf(radiansZ * 0.5f);
		float sz = sinf(radiansZ * 0.5f);

		// q = qY * qX * qZ. X rotates nose up/down, Y turns left/right, Z
		// rolls the head clockwise.
		return Quaternion(
			sx * cy * cz - cx * sy * sz,
			cx * sy * cz + sx * cy * sz,
			cx * cy * sz - sx * sy * cz,
			cx * cy * cz + sx * sy * sz);
	}

	/**
	 * Creates a rotation quaternion such that applying it rotates the -Z
	 * axis to point at the given target direction.
	 **/
	static Quaternion lookRotation(const Vector3 &forward, const Vector3 &up)
	{
		Vector3 f = forward.normalized();
		Vector3 u = up.normalized();

		// Orthonormal basis for a -Z-forward view (gluLookAt convention):
		// right = forward x up, true up = right x forward.
		Vector3 r = f.cross(u);
		if (r.lengthsq() > 0.0f)
			r.normalize();
		else
		{
			// forward is (anti)parallel to up; fall back to any
			// perpendicular up so the basis stays valid.
			Vector3 alt = (fabsf(f.y) < 0.9f) ? Vector3(0.0f, 1.0f, 0.0f) : Vector3(1.0f, 0.0f, 0.0f);
			r = f.cross(alt).normalized();
		}
		u = r.cross(f);

		// Build the rotation matrix whose COLUMNS are the rotated basis
		// vectors (mat(col, row) addresses m[col * 4 + row]): +X maps to
		// right, +Y maps to up, +Z maps to -forward, so the local -Z axis
		// (the view direction) points along `forward`.
		Matrix4 mat = Matrix4::identity();
		mat(0, 0) = r.x;  mat(0, 1) = r.y;  mat(0, 2) = r.z;  // column 0
		mat(1, 0) = u.x;  mat(1, 1) = u.y;  mat(1, 2) = u.z;  // column 1
		mat(2, 0) = -f.x; mat(2, 1) = -f.y; mat(2, 2) = -f.z; // column 2

		// From a rotation matrix:
		float trace = mat(0, 0) + mat(1, 1) + mat(2, 2);
		Quaternion q;
		if (trace > 0.0f)
		{
			float s = 0.5f / sqrtf(trace + 1.0f);
			q.w = 0.25f / s;
			q.x = (mat(1, 2) - mat(2, 1)) * s;
			q.y = (mat(2, 0) - mat(0, 2)) * s;
			q.z = (mat(0, 1) - mat(1, 0)) * s;
		}
		else if (mat(0, 0) > mat(1, 1) && mat(0, 0) > mat(2, 2))
		{
			float s = 2.0f * sqrtf(1.0f + mat(0, 0) - mat(1, 1) - mat(2, 2));
			q.w = (mat(2, 1) - mat(1, 2)) / s;
			q.x = 0.25f * s;
			q.y = (mat(1, 0) + mat(0, 1)) / s;
			q.z = (mat(2, 0) + mat(0, 2)) / s;
		}
		else if (mat(1, 1) > mat(2, 2))
		{
			float s = 2.0f * sqrtf(1.0f + mat(1, 1) - mat(0, 0) - mat(2, 2));
			q.w = (mat(2, 0) - mat(0, 2)) / s;
			q.x = (mat(1, 0) + mat(0, 1)) / s;
			q.y = 0.25f * s;
			q.z = (mat(2, 1) + mat(1, 2)) / s;
		}
		else
		{
			float s = 2.0f * sqrtf(1.0f + mat(2, 2) - mat(0, 0) - mat(1, 1));
			q.w = (mat(1, 0) - mat(0, 1)) / s;
			q.x = (mat(2, 0) + mat(0, 2)) / s;
			q.y = (mat(2, 1) + mat(1, 2)) / s;
			q.z = 0.25f * s;
		}

		return q.normalized();
	}

	/**
	 * Extract rotation angles (in radians) such that
	 * fromEuler(x, y, z) reproduces this quaternion, when possible.
	 **/
	void toEuler(float &radiansX, float &radiansY, float &radiansZ) const
	{
		// Roll (z), pitch (x), yaw (y).
		float test = x * y + z * w;
		if (test > 0.499f)
		{
			// Singularity at north pole.
			radiansZ = 2.0f * atan2f(x, w);
			radiansY = 0.0f;
			radiansX = 3.14159265358979f * 0.5f;
		}
		else if (test < -0.499f)
		{
			// Singularity at south pole.
			radiansZ = -2.0f * atan2f(x, w);
			radiansY = 0.0f;
			radiansX = -3.14159265358979f * 0.5f;
		}
		else
		{
			float sqx = x * x, sqy = y * y, sqz = z * z;
			radiansX = atan2f(2.0f * y * w - 2.0f * x * z, 1.0f - 2.0f * sqy - 2.0f * sqz);
			radiansZ = asinf(2.0f * test);
			radiansY = atan2f(2.0f * x * w - 2.0f * y * z, 1.0f - 2.0f * sqx - 2.0f * sqz);
		}
	}

	inline Vector3 toEulerAngleVector() const
	{
		float rx, ry, rz;
		toEuler(rx, ry, rz);
		return Vector3(rx, ry, rz);
	}

	/**
	 * The forward direction: the -Z axis rotated by this quaternion.
	 **/
	inline Vector3 forward() const
	{
		return rotateVector(Vector3(0, 0, -1));
	}

	/**
	 * The up direction: the +Y axis rotated by this quaternion.
	 **/
	inline Vector3 up() const
	{
		return rotateVector(Vector3(0, 1, 0));
	}

	/**
	 * The right direction: the +X axis rotated by this quaternion.
	 **/
	inline Vector3 right() const
	{
		return rotateVector(Vector3(1, 0, 0));
	}

	/**
	 * Rotates a vector by this quaternion. This assumes the quaternion is
	 * normalized for meaningful results.
	 **/
	inline Vector3 rotateVector(const Vector3 &v) const
	{
		// v' = q v q* -- expanded to the efficient 3-form:
		Vector3 u(x, y, z);
		Vector3 c = u.cross(v) * 2.0f;
		return v + c * w + u.cross(c);
	}

	/**
	 * Spherical linear interpolation.
	 **/
	static Quaternion slerp(const Quaternion &a, const Quaternion &b, float t)
	{
		// Clamp to [0, 1] and nudge the worst-case parity so the resultant
		// path is always the shortest one.
		Quaternion target = b;
		float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

		if (dot < 0.0f)
		{
			target = Quaternion(-b.x, -b.y, -b.z, -b.w);
			dot = -dot;
		}

		if (dot > 0.9995f)
			return Quaternion(a + (target - a) * t).normalized();

		dot = fminf(1.0f, fmaxf(0.0f, dot));
		float theta0 = acosf(dot);
		float theta = theta0 * t;
		float sintheta = sinf(theta);
		float sintheta0 = sinf(theta0);

		float s1 = cosf(theta) - dot * sintheta / sintheta0;
		float s2 = sintheta / sintheta0;
		Quaternion q = a * s1 + target * s2;
		return q.normalized();
	}

	/**
	 * Nudge-towards-equal interpolation (nlerp) for smooth camera blending.
	 **/
	static Quaternion nlerp(const Quaternion &a, const Quaternion &b, float t)
	{
		Quaternion target = b;
		float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
		if (dot < 0.0f)
			target = Quaternion(-b.x, -b.y, -b.z, -b.w);
		Quaternion q = a + (target - a) * t;
		return q.normalized();
	}

	/**
	 * Axis angle representation.
	 **/
	void toAxisAngle(Vector3 &axis, float &radians) const
	{
		float len = sqrtf(x * x + y * y + z * z);
		if (len > 0.0f)
		{
			axis = Vector3(x / len, y / len, z / len);
			radians = 2.0f * acosf(w);
		}
		else
		{
			axis = Vector3(0, 1, 0);
			radians = 0.0f;
		}
	}

	inline Quaternion operator + (const Quaternion &q) const
	{
		return Quaternion(x + q.x, y + q.y, z + q.z, w + q.w);
	}

	inline Quaternion operator - (const Quaternion &q) const
	{
		return Quaternion(x - q.x, y - q.y, z - q.z, w - q.w);
	}

	/**
	 * Scaled version (also used as a normalized const).
	 **/
	inline Quaternion operator * (float s) const
	{
		return Quaternion(x * s, y * s, z * s, w * s);
	}

	/**
	 * Quaternion conjugation (inverse rotation for unit quaternions).
	 **/
	inline Quaternion conjugate() const
	{
		return Quaternion(-x, -y, -z, w);
	}

	/**
	 * Hamilton product:
	 *   Applying *this, then applying q, is the same as ((*this) * q).
	 * The result may need normalizing afterwards.
	 **/
	Quaternion operator * (const Quaternion &q) const
	{
		return Quaternion(
			w * q.x + x * q.w + y * q.z - z * q.y,
			w * q.y - x * q.z + y * q.w + z * q.x,
			w * q.z + x * q.y - y * q.x + z * q.w,
			w * q.w - x * q.x - y * q.y - z * q.z);
	}

	/**
	 * Rotational composition.
	 **/
	inline Quaternion &operator *= (const Quaternion &q)
	{
		*this = *this * q;
		return *this;
	}

	/**
	 * Normalized copy. Leaves zero-quaternions alone.
	 **/
	inline Quaternion normalized() const
	{
		float len = sqrtf(x * x + y * y + z * z + w * w);
		if (len > 0.0f)
			return Quaternion(x / len, y / len, z / len, w / len);
		return *this;
	}

	inline void normalize() { *this = normalized(); }

	inline float dot(const Quaternion &q) const
	{
		return x * q.x + y * q.y + z * q.z + w * q.w;
	}

	/**
	 * Builds the 4x4 rotation-only matrix for this quaternion.
	 **/
	Matrix4 toMatrix() const
	{
		Matrix4 mat = Matrix4::identity();
		float xx = x * x, xy = x * y, xz = x * z;
		float yy = y * y, yz = y * z, zz = z * z;
		float wx = w * x, wy = w * y, wz = w * z;

		// Standard rotation matrix (column-vector convention): column 0 is
		// the rotated +X axis, column 1 the rotated +Y axis, column 2 the
		// rotated +Z axis. mat(col, row) addresses m[col * 4 + row].
		mat(0, 0) = 1.0f - 2.0f * (yy + zz);
		mat(0, 1) = 2.0f * (xy + wz);
		mat(0, 2) = 2.0f * (xz - wy);

		mat(1, 0) = 2.0f * (xy - wz);
		mat(1, 1) = 1.0f - 2.0f * (xx + zz);
		mat(1, 2) = 2.0f * (yz + wx);

		mat(2, 0) = 2.0f * (xz + wy);
		mat(2, 1) = 2.0f * (yz - wx);
		mat(2, 2) = 1.0f - 2.0f * (xx + yy);

		return mat;
	}

	/**
	 * Creates a quaternion from just a rotation matrix's top-left 3x3
	 * section of m.
	 **/
	static Quaternion fromMatrix(const Matrix4 &mat)
	{
		return lookRotation(mat.transformDirection(Vector3(0, 0, -1)), mat.transformDirection(Vector3(0, 1, 0)));
	}

}; // Quaternion

} // graphics3d
} // love
