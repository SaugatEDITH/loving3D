/**
 * LOVING3D - 3D math utilities for the LOVE game engine.
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
 * 3D mathematics utilities for LOVING3D.
 *
 * Coordinate system convention (right-handed):
 *   +X points right, +Y points up, +Z points "out of the screen" toward the
 *   viewer. Cameras by default look down their local -Z axis like OpenGL,
 *   Metal and Vulkan all arrange their vertex-independent depth such that
 *   increasing Z is further away.
 *
 * The 2D love.graphics coordinate system (y-down) is completely independent
 * from this 3D coordinate system; 3D values are never transformed through the
 * 2D transform. See graphics3d/Graphics3D.cpp state handling details.
 **/

#pragma once

#include <cmath>
#include <cstddef>

namespace love
{
namespace graphics3d
{

/**
 * A 3-component floating point vector.
 **/
struct Vector3
{
	float x, y, z;

	inline Vector3()
		: x(0.0f), y(0.0f), z(0.0f)
	{}

	inline Vector3(float v)
		: x(v), y(v), z(v)
	{}

	inline Vector3(float xx, float yy, float zz)
		: x(xx), y(yy), z(zz)
	{}

	inline operator float *() { return &x; }
	inline operator const float *() const { return &x; }

	inline bool operator == (const Vector3 &v) const
	{
		return x == v.x && y == v.y && z == v.z;
	}

	inline bool operator != (const Vector3 &v) const
	{
		return !(*this == v);
	}

	inline Vector3 operator + (const Vector3 &v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
	inline Vector3 operator - (const Vector3 &v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
	inline Vector3 operator * (const Vector3 &v) const { return Vector3(x * v.x, y * v.y, z * v.z); }
	inline Vector3 operator / (const Vector3 &v) const { return Vector3(x / v.x, y / v.y, z / v.z); }

	inline Vector3 operator * (float s) const { return Vector3(x * s, y * s, z * s); }
	inline Vector3 operator / (float s) const { return Vector3(x / s, y / s, z / s); }

	inline Vector3 operator - () const { return Vector3(-x, -y, -z); }

	inline Vector3 &operator += (const Vector3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
	inline Vector3 &operator -= (const Vector3 &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	inline Vector3 &operator *= (const Vector3 &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
	inline Vector3 &operator /= (const Vector3 &v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
	inline Vector3 &operator *= (float s) { x *= s; y *= s; z *= s; return *this; }
	inline Vector3 &operator /= (float s) { x /= s; y /= s; z /= s; return *this; }

	inline float length() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	inline float lengthsq() const
	{
		return x * x + y * y + z * z;
	}

	/**
	 * Returns a unit-length copy of this vector. Zero vectors are returned
	 * unchanged.
	 **/
	inline Vector3 normalized() const
	{
		float len = length();
		if (len > 0.0f)
			return this->operator/ (len);
		return *this;
	}

	inline void normalize() { *this = normalized(); }

	/**
	 * Linear interpolation between two vectors.
	 **/
	inline static Vector3 lerp(const Vector3 &a, const Vector3 &b, float t)
	{
		return Vector3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
	}

	/**
	 * Smoother Hermite interpolation (smoothstep ease on t).
	 **/
	inline static Vector3 smoothLerp(const Vector3 &a, const Vector3 &b, float t)
	{
		t = t * t * (3.0f - 2.0f * t);
		return lerp(a, b, t);
	}

	/**
	 * Spherical linear interpolation.
	 **/
	inline static Vector3 slerp(const Vector3 &a, const Vector3 &b, float t)
	{
		float dot = a.normalized().dot(b.normalized());
		dot = fmaxf(-1.0f, fminf(1.0f, dot));
		float theta = acosf(dot) * t;

		Vector3 relative = b.normalized() - a.normalized() * dot;
		relative.normalize();

		return (a.normalized() * cosf(theta)) + (relative * sinf(theta));
	}

	inline float dot(const Vector3 &v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}

	inline Vector3 cross(const Vector3 &v) const
	{
		return Vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}

	/**
	 * Distance between two points in space.
	 **/
	inline static float distance(const Vector3 &a, const Vector3 &b)
	{
		return (a - b).length();
	}

	/**
	 * Reflection of this vector about a surface normal.
	 **/
	inline Vector3 reflect(const Vector3 &n) const
	{
		return *this - n * (this->dot(n) * 2.0f);
	}

	/**
	 * Project this vector onto another vector.
	 **/
	inline Vector3 project(const Vector3 &onto) const
	{
		float lensq = onto.lengthsq();
		if (lensq > 0.0f)
			return onto * (this->dot(onto) / lensq);
		return Vector3();
	}

	/**
	 * Direction from this point to another point.
	 **/
	inline Vector3 direction(const Vector3 &to) const
	{
		return (to - *this).normalized();
	}
};

/**
 * A 4-component floating point vector.
 **/
struct Vector4
{
	float x, y, z, w;

	inline Vector4()
		: x(0.0f), y(0.0f), z(0.0f), w(0.0f)
	{}

	inline Vector4(float v)
		: x(v), y(v), z(v), w(v)
	{}

	inline Vector4(float xx, float yy, float zz, float ww)
		: x(xx), y(yy), z(zz), w(ww)
	{}

	inline Vector4(const Vector3 &v, float ww)
		: x(v.x), y(v.y), z(v.z), w(ww)
	{}

	inline operator float *() { return &x; }
	inline operator const float *() const { return &x; }

	inline bool operator == (const Vector4 &v) const
	{
		return x == v.x && y == v.y && z == v.z && w == v.w;
	}

	inline bool operator != (const Vector4 &v) const
	{
		return !(*this == v);
	}

	inline Vector4 operator + (const Vector4 &v) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }
	inline Vector4 operator - (const Vector4 &v) const { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }
	inline Vector4 operator * (const Vector4 &v) const { return Vector4(x * v.x, y * v.y, z * v.z, w * v.w); }
	inline Vector4 operator / (const Vector4 &v) const { return Vector4(x / v.x, y / v.y, z / v.z, w / v.w); }

	inline Vector4 operator * (float s) const { return Vector4(x * s, y * s, z * s, w * s); }
	inline Vector4 operator / (float s) const { return Vector4(x / s, y / s, z / s, w / s); }

	inline Vector4 operator - () const { return Vector4(-x, -y, -z, -w); }

	inline Vector4 &operator += (const Vector4 &v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	inline Vector4 &operator -= (const Vector4 &v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	inline Vector4 &operator *= (float s) { x *= s; y *= s; z *= s; w *= s; return *this; }
	inline Vector4 &operator /= (float s) { x /= s; y /= s; z /= s; w /= s; return *this; }

	inline float dot(const Vector4 &v) const
	{
		return x * v.x + y * v.y + z * v.z + w * v.w;
	}

	inline float length() const
	{
		return sqrtf(x * x + y * y + z * z + w * w);
	}

	inline Vector4 normalized() const
	{
		float len = length();
		if (len > 0.0f)
			return this->operator/ (len);
		return *this;
	}

	inline void normalize() { *this = normalized(); }

	/**
	 * Linear interpolation between two vectors.
	 **/
	inline static Vector4 lerp(const Vector4 &a, const Vector4 &b, float t)
	{
		return Vector4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
	}
};

} // graphics3d
} // love
