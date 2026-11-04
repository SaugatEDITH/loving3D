/**
 * Copyright (c) 2006-2026 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
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

#include "Matrix4.h"
#include "Quaternion.h"

#include <limits>

namespace love
{
namespace graphics3d
{

/**
 * Computes matrix inverse using cofactors. General-purpose safe inverse.
 **/
Matrix4 Matrix4::inverse() const
{
	float inv[16];

	inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
	inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
	inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
	inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
	inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
	inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
	inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
	inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
	inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
	inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
	inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
	inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
	inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
	inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
	inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
	inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

	float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	Matrix4 result;
	result.setZero();

	if (det != 0.0f)
	{
		float invdet = 1.0f / det;
		for (int i = 0; i < 16; i++)
			result.m[i] = inv[i] * invdet;
	}

	return result;
}

Matrix4 Matrix4::inverseRigid() const
{
	// For a rotation R and translation t: inverse = R^T * -R^T*t.
	Matrix4 result;

	// Transpose the upper-left 3x3 (rotation) block.
	result.m[0] = m[0];  result.m[4] = m[1];  result.m[8]  = m[2];  result.m[12] = 0.0f;
	result.m[1] = m[4];  result.m[5] = m[5];  result.m[9]  = m[6];  result.m[13] = 0.0f;
	result.m[2] = m[8];  result.m[6] = m[9];  result.m[10] = m[10]; result.m[14] = 0.0f;
	result.m[3] = 0.0f;  result.m[7] = 0.0f;  result.m[11] = 0.0f;  result.m[15] = 1.0f;

	// Translation part: -R^T * t.
	float tx = m[12], ty = m[13], tz = m[14];
	result.m[12] = -(result.m[0] * tx + result.m[4] * ty + result.m[8] * tz);
	result.m[13] = -(result.m[1] * tx + result.m[5] * ty + result.m[9] * tz);
	result.m[14] = -(result.m[2] * tx + result.m[6] * ty + result.m[10] * tz);

	return result;
}

Matrix4 Matrix4::transposed() const
{
	Matrix4 r = *this;
	for (int c = 0; c < 4; c++)
	{
		for (int row = c + 1; row < 4; row++)
		{
			float tmp = r.m[c * 4 + row];
			r.m[c * 4 + row] = r.m[row * 4 + c];
			r.m[row * 4 + c] = tmp;
		}
	}
	return r;
}

Matrix3 Matrix4::normalMatrix() const
{
	// transpose(inverse(model3x3)) transforms normals from model space into
	// world space for non-uniform scale, and equals the identity rotation for
	// uniform scale.
	Matrix3 n;
	n.m[0] = m[0]; n.m[3] = m[4]; n.m[6] = m[8];
	n.m[1] = m[1]; n.m[4] = m[5]; n.m[7] = m[9];
	n.m[2] = m[2]; n.m[5] = m[6]; n.m[8] = m[10];
	return n.inverse().transposed();
}

Matrix3 Matrix3::rotationAboutAxis(const Vector3 &axis, float angle)
{
	Vector3 a = axis.normalized();
	Matrix3 result = identity();

	float c = cosf(angle);
	float s = sinf(angle);
	float t = 1.0f - c;

	float cm2 = 1.0f - a.x * a.x;

	// Set the 3x3 rotation matrix via the axis-angle (Rodrigues) formula.
	result(0, 0) = t * a.x * a.x + c;
	result(0, 1) = t * a.x * a.y + s * a.z;
	result(0, 2) = t * a.x * a.z - s * a.y;

	result(1, 0) = t * a.x * a.y - s * a.z;
	result(1, 1) = t * a.y * a.y + c;
	result(1, 2) = t * a.y * a.z + s * a.x;

	result(2, 0) = t * a.x * a.z + s * a.y;
	result(2, 1) = t * a.y * a.z - s * a.x;
	result(2, 2) = t * a.z * a.z + c;

	(void) cm2;
	return result;
}

Matrix3 Matrix3::inverse() const
{
	Matrix3 inv;

	float a11 = m[0], a12 = m[3], a13 = m[6];
	float a21 = m[1], a22 = m[4], a23 = m[7];
	float a31 = m[2], a32 = m[5], a33 = m[8];

	float det = a11 * (a22 * a33 - a23 * a32) - a21 * (a12 * a33 - a13 * a32) + a31 * (a12 * a23 - a13 * a22);
	if (det == 0.0f)
		return Matrix3::identity();

	float invdet = 1.0f / det;

	inv.m[0] = (a22 * a33 - a23 * a32) * invdet;
	inv.m[3] = -(a12 * a33 - a13 * a32) * invdet;
	inv.m[6] = (a12 * a23 - a13 * a22) * invdet;
	inv.m[1] = -(a21 * a33 - a23 * a31) * invdet;
	inv.m[4] = (a11 * a33 - a13 * a31) * invdet;
	inv.m[7] = -(a11 * a23 - a13 * a21) * invdet;
	inv.m[2] = (a21 * a32 - a22 * a31) * invdet;
	inv.m[5] = -(a11 * a32 - a12 * a31) * invdet;
	inv.m[8] = (a11 * a22 - a12 * a21) * invdet;

	return inv;
}

void Matrix4::setTranslation(const Vector3 &t)
{
	setIdentity();
	m[12] = t.x;
	m[13] = t.y;
	m[14] = t.z;
}

void Matrix4::setRotationEuler(const Vector3 &degrees)
{
	setIdentity();
	*this = rotationZ(degrees.z) * rotationX(degrees.x) * rotationY(degrees.y);
}

void Matrix4::setScaling(const Vector3 &s)
{
	setIdentity();
	m[0] = s.x;
	m[5] = s.y;
	m[10] = s.z;
}

void Matrix4::setTransform(const Vector3 &position, const Vector3 &eulerAngles, const Vector3 &scale)
{
	setIdentity();
	*this = rotationZ(eulerAngles.z) * rotationX(eulerAngles.x) * rotationY(eulerAngles.y) * scaling(scale);
	m[12] = position.x;
	m[13] = position.y;
	m[14] = position.z;
}

void Matrix4::setTransformQ(const Vector3 &position, const Quaternion &q, const Vector3 &scale)
{
	Matrix4 rotation = q.toMatrix();
	rotation = rotation * scaling(scale);
	rotation.m[12] = position.x;
	rotation.m[13] = position.y;
	rotation.m[14] = position.z;
	*this = rotation;
}

Vector3 Matrix4::extractPosition() const
{
	return Vector3(m[12], m[13], m[14]);
}

Matrix4 Matrix4::translation(const Vector3 &t)
{
	Matrix4 r;
	r.setIdentity();
	r.m[12] = t.x;
	r.m[13] = t.y;
	r.m[14] = t.z;
	return r;
}

Matrix4 Matrix4::scaling(const Vector3 &s)
{
	Matrix4 r;
	r.setIdentity();
	r.m[0] = s.x;
	r.m[5] = s.y;
	r.m[10] = s.z;
	return r;
}

Matrix4 Matrix4::rotationX(float radians)
{
	Matrix4 r;
	r.setIdentity();
	float c = cosf(radians);
	float s = sinf(radians);
	r.m[5] = c;  r.m[9] = -s;
	r.m[6] = s;  r.m[10] = c;
	return r;
}

Matrix4 Matrix4::rotationY(float radians)
{
	Matrix4 r;
	r.setIdentity();
	float c = cosf(radians);
	float s = sinf(radians);
	r.m[0] = c;  r.m[8] = s;
	r.m[2] = -s; r.m[10] = c;
	return r;
}

Matrix4 Matrix4::rotationZ(float radians)
{
	Matrix4 r;
	r.setIdentity();
	float c = cosf(radians);
	float s = sinf(radians);
	r.m[0] = c; r.m[4] = -s;
	r.m[1] = s; r.m[5] = c;
	return r;
}

Matrix4 Matrix4::rotationAboutAxis(const Vector3 &axis, float radians)
{
	Vector3 a = axis.normalized();
	Matrix4 r;
	r.setIdentity();

	float c = cosf(radians);
	float s = sinf(radians);
	float t = 1.0f - c;

	r.m[0] = t * a.x * a.x + c;
	r.m[4] = t * a.x * a.y - s * a.z;
	r.m[8] = t * a.x * a.z + s * a.y;

	r.m[1] = t * a.x * a.y + s * a.z;
	r.m[5] = t * a.y * a.y + c;
	r.m[9] = t * a.y * a.z - s * a.x;

	r.m[2] = t * a.x * a.z - s * a.y;
	r.m[6] = t * a.y * a.z + s * a.x;
	r.m[10] = t * a.z * a.z + c;

	return r;
}

Matrix4 Matrix4::lookAt(const Vector3 &eye, const Vector3 &target, const Vector3 &up)
{
	// Right-handed view matrix: the camera looks down -Z in view space.
	Vector3 f = (eye - target).normalized(); // "backward" = away from target
	Vector3 s = up.cross(f).normalized();    // right axis
	Vector3 u = f.cross(s);                  // camera up

	Matrix4 r;
	r.m[0] = s.x;  r.m[4] = s.y;  r.m[8] = s.z;  r.m[12] = -s.dot(eye);
	r.m[1] = u.x;  r.m[5] = u.y;  r.m[9] = u.z;  r.m[13] = -u.dot(eye);
	r.m[2] = f.x;  r.m[6] = f.y;  r.m[10] = f.z; r.m[14] = -f.dot(eye);
	r.m[3] = 0.0f; r.m[7] = 0.0f; r.m[11] = 0.0f; r.m[15] = 1.0f;

	return r;
}

Matrix4 Matrix4::perspective(float fovYRadians, float aspect, float near, float far)
{
	Matrix4 r;
	r.setZero();

	float tanHalfFovY = tanf(fovYRadians * 0.5f);

	if (tanHalfFovY <= 0.0f || near <= 0.0f || far <= near)
		return r;

	r.m[0] = 1.0f / (aspect * tanHalfFovY);
	r.m[5] = 1.0f / tanHalfFovY;

	// NDC z range [-1, +1], with -near mapping to -1, -far mapping to +1.
	r.m[10] = -(far + near) / (far - near);
	r.m[11] = -1.0f; // column 2, row 3: -w, w = z_view, so negative z_view -> +w.
	r.m[14] = -(2.0f * far * near) / (far - near);

	return r;
}

Matrix4 Matrix4::orthographic(float left, float right, float bottom, float top, float near, float far)
{
	Matrix4 r;
	r.setZero();

	if (right <= left || top <= bottom || far <= near)
		return r;

	r.m[0] = 2.0f / (right - left);
	r.m[5] = 2.0f / (top - bottom);
	r.m[10] = -2.0f / (far - near);
	r.m[12] = -(left + right) / (right - left);
	r.m[13] = -(top + bottom) / (top - bottom);
	r.m[14] = -(far + near) / (far - near);
	r.m[15] = 1.0f;

	return r;
}

} // graphics3d
} // love
