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
 * 4x4 and 3x3 matrices with column-major memory layout, ideal for direct
 * upload to GPU uniform buffers (matching GLSL's native mat4 layout).
 *
 * These live in the love::graphics3d namespace and are distinct from
 * love::Matrix4, love's 2D-flavored transform matrix.
 *
 * Conventions (right-handed):
 *   - Matrix-vector multiplication: v' = M * v (column vectors).
 *   - Model -> View -> Projection transform order.
 *   - The camera looks down its local -Z axis.
 **/

#pragma once

#include "Vector3.h"
#include <cmath>
#include <limits>
#include <cstddef>

namespace love
{
namespace graphics3d
{

/**
 * A 3x3 matrix. Useful for rotation tensors and the normal matrix.
 * Memory is laid out column-major: m[x * 3 + y].
 **/
class Matrix3
{
public:

	float m[9];

	inline Matrix3()
	{
		setIdentity();
	}

	inline void setIdentity()
	{
		m[0] = 1.0f; m[3] = 0.0f; m[6] = 0.0f;
		m[1] = 0.0f; m[4] = 1.0f; m[7] = 0.0f;
		m[2] = 0.0f; m[5] = 0.0f; m[8] = 1.0f;
	}

	inline float &operator () (int col, int row) { return m[col * 3 + row]; }
	inline float operator () (int col, int row) const { return m[col * 3 + row]; }

	inline Matrix3 operator * (const Matrix3 &o) const
	{
		Matrix3 result = identity();
		for (int c = 0; c < 3; c++)
		{
			for (int r = 0; r < 3; r++)
			{
				float s = 0.0f;
				for (int k = 0; k < 3; k++)
					s += (*this)(k, r) * o(c, k);
				result(c, r) = s;
			}
		}
		return result;
	}

	inline Matrix3 &operator *= (const Matrix3 &o)
	{
		*this = *this * o;
		return *this;
	}

	inline Vector3 operator * (const Vector3 &v) const
	{
		return Vector3(
			m[0] * v.x + m[3] * v.y + m[6] * v.z,
			m[1] * v.x + m[4] * v.y + m[7] * v.z,
			m[2] * v.x + m[5] * v.y + m[8] * v.z);
	}

	/**
	 * Returns the inverse of this matrix. Assumes the matrix represents a
	 * rotation-scale (orthogonal-ish) transform for efficiency, but falls
	 * back to full cofactor inversion for general matrices.
	 **/
	Matrix3 inverse() const;

	inline float determinant() const
	{
		return m[0] * (m[4] * m[8] - m[5] * m[7])
			- m[3] * (m[1] * m[8] - m[2] * m[7])
			+ m[6] * (m[1] * m[5] - m[2] * m[4]);
	}

	/**
	 * Returns the transpose of this matrix. For rotation matrices
	 * transpose == inverse.
	 **/
	inline Matrix3 transposed() const
	{
		Matrix3 r = *this;
		r.m[1] = m[3]; r.m[3] = m[1];
		r.m[2] = m[6]; r.m[6] = m[2];
		r.m[5] = m[7]; r.m[7] = m[5];
		return r;
	}

	inline static Matrix3 identity()
	{
		Matrix3 r;
		return r;
	}

	static Matrix3 rotationAboutAxis(const Vector3 &axis, float angle);
};

/**
 * A 4x4 matrix with a full transform API set: building model, view, and
 * projection matrices... contains inverse(), transposes, and composition.
 **/
class Matrix4
{
public:

	// Column-major storage: m[col * 4 + row].
	float m[16];

	inline Matrix4()
	{
		setIdentity();
	}

	inline void setIdentity()
	{
		m[0] = 1.0f; m[4] = 0.0f; m[8] = 0.0f;  m[12] = 0.0f;
		m[1] = 0.0f; m[5] = 1.0f; m[9] = 0.0f;  m[13] = 0.0f;
		m[2] = 0.0f; m[6] = 0.0f; m[10] = 1.0f; m[14] = 0.0f;
		m[3] = 0.0f; m[7] = 0.0f; m[11] = 0.0f; m[15] = 1.0f;
	}

	/**
	 * Returns a new identity matrix.
	 **/
	inline static Matrix4 identity()
	{
		Matrix4 r;
		r.setIdentity();
		return r;
	}

	inline float &operator () (int col, int row) { return m[col * 4 + row]; }
	inline float operator () (int col, int row) const { return m[col * 4 + row]; }

	inline operator float *() { return m; }
	inline operator const float *() const { return m; }

	/**
	 * Matrix multiplication: *this = *this * o.
	 * Returns a new matrix (does not mutate either input).
	 **/
	inline static Matrix4 mul(const Matrix4 &a, const Matrix4 &b)
	{
		Matrix4 result;
		result.setZero();
		for (int c = 0; c < 4; c++)
		{
			for (int r = 0; r < 4; r++)
			{
				float s = 0.0f;
				for (int k = 0; k < 4; k++)
					s += a.m[k * 4 + r] * b.m[c * 4 + k];
				result.m[c * 4 + r] = s;
			}
		}
		return result;
	}

	inline Matrix4 operator * (const Matrix4 &o) const { return mul(*this, o); }

	// In-place composition: this = this * o.
	inline Matrix4 &operator *= (const Matrix4 &o)
	{
		*this = *this * o;
		return *this;
	}

	/**
	 * Transforms a point as a vec4 with the given w component.
	 **/
	inline Vector4 operator * (const Vector4 &v) const
	{
		return Vector4(
			m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
			m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
			m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
			m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w);
	}

	/**
	 * Transforms a point (w = 1). Results are not divided by w (no
	 * perspective projection here).
	 **/
	inline Vector3 operator * (const Vector3 &v) const
	{
		return Vector3(
			m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12],
			m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13],
			m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14]);
	}

	/**
	 * Transforms a direction (w = 0, i.e. translation ignored).
	 **/
	inline Vector3 transformDirection(const Vector3 &v) const
	{
		return Vector3(
			m[0] * v.x + m[4] * v.y + m[8] * v.z,
			m[1] * v.x + m[5] * v.y + m[9] * v.z,
			m[2] * v.x + m[6] * v.y + m[10] * v.z);
	}

	/**
	 * Un-project after perspective divide (w division done by caller or
	 * internally, when isWDivide is passed true).
	 **/
	inline Vector3 projectPoint(const Vector3 &v, bool divideW = false) const
	{
		Vector4 h = *this * Vector4(v, 1.0f);
		if (divideW && h.w != 0.0f)
			return Vector3(h.x / h.w, h.y / h.w, h.z / h.w);
		return Vector3(h.x, h.y, h.z);
	}

	/**
	 * Sets a complete transformation (builds T * R * S in one go).
	 * Note the final matrix order is T * R * S: scale first, rotate, then
	 * translate.
	 **/
	void setTransform(const Vector3 &position, const Vector3 &eulerAngles, const Vector3 &scale);

	/**
	 * Sets a complete transformation using... a quaternion for rotation.
	 **/
	void setTransformQ(const Vector3 &position, const class Quaternion &q, const Vector3 &scale);

	void setTranslation(const Vector3 &t);
	void setRotationEuler(const Vector3 &degrees);
	void setScaling(const Vector3 &s);

	Vector3 extractPosition() const;

	/**
	 * Builds a matrix that applies this matrix's rotation + translation +
	 * scaling composed onto the given local-space point.
	 **/
	static Matrix4 translation(const Vector3 &t);
	static Matrix4 scaling(const Vector3 &s);
	static Matrix4 rotationX(float radians);
	static Matrix4 rotationY(float radians);
	static Matrix4 rotationZ(float radians);
	static Matrix4 rotationAboutAxis(const Vector3 &axis, float radians);

	/**
	 * Builds an orthonormal basis matrix transforming from a camera in
	 * "eye" state into "view / camera" space (the view matrix).
	 **/
	static Matrix4 lookAt(const Vector3 &eye, const Vector3 &target, const Vector3 &up);

	/**
	 * Right-handed perspective projection; -Z is the "into screen"
	 * direction. Depth maps to NDC z in [-1, 1]:
	 *   (-Znear -> -1, -Zfar -> +1.)
	 **/
	static Matrix4 perspective(float fovYRadians, float aspect, float near, float far);

	/**
	 * Right-handed orthographic projection with NDC z in [-1, 1].
	 **/
	static Matrix4 orthographic(float left, float right, float bottom, float top, float near, float far);

	/**
	 * Augments this transform with a world-space transformation: *this =
	 * world * this.
	 **/
	void applyWorldMatrix(const Matrix4 &world)
	{
		*this = mul(world, *this);
	}

	/**
	 * "Removes" (multiplies out) a child transform: this = this * inverse(child),
	 * keeping this matrix in the parent's coordinates.
	 **/
	void subtractChildTransform(const Matrix4 &child)
	{
		*this = mul(*this, child.inverse());
	}

	Matrix4 transposed() const;

	/**
	 * Computes the full 4x4 inverse using cofactor expansion.
	 * Hasperformance note: it's fast enough (one matrix per object per
	 * frame at most) and mathematically safe for general matrices.
	 **/
	Matrix4 inverse() const;

	/**
	 * Inverse assuming the matrix is a rigid transform (pure rotation +
	 * translation; scale == 1). Fast path for camera transforms.
	 **/
	Matrix4 inverseRigid() const;

	/**
	 * Builds a normal matrix from this model matrix's 3x3 upper block
	 * (the "world-space transforms normals correctly" matrix,
	 * transpose(inverse(model))).
	 **/
	Matrix3 normalMatrix() const;

	/**
	 * Sets all elements to zero (not identity!).
	 **/
	void setZero()
	{
		for (int i = 0; i < 16; i++)
			m[i] = 0.0f;
	}

private:

	// < helper constant for fast inverse() cofactor storage layout.
};

} // graphics3d
} // love
