/**
 * LOVING3D - primitive shape generator implementation.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org). The underlying engine
 * remains freely distributable under the terms of its original zlib license
 * (see license.txt).
 **/

#include "Primitives.h"

#include <cmath>
#include <algorithm>

namespace love
{
namespace graphics3d
{

namespace primitives
{

namespace
{

const float PI = 3.14159265359f;

void addVertex(PrimitiveGeometry &out, const Vector3 &p, const Vector3 &n, float u, float v)
{
	Vertex3D vert;
	vert.position[0] = p.x;
	vert.position[1] = p.y;
	vert.position[2] = p.z;
	vert.normal[0] = n.x;
	vert.normal[1] = n.y;
	vert.normal[2] = n.z;
	vert.texcoord[0] = u;
	vert.texcoord[1] = v;
	vert.color[0] = 1.0f;
	vert.color[1] = 1.0f;
	vert.color[2] = 1.0f;
	vert.color[3] = 1.0f;
	out.vertices.push_back(vert);
}

void addTriangle(PrimitiveGeometry &out, uint32 a, uint32 b, uint32 c, bool flip = false)
{
	if (flip)
	{
		out.indices.insert(out.indices.end(), { a, c, b });
	}
	else
	{
		out.indices.insert(out.indices.end(), { a, b, c });
	}
}

void addQuad(PrimitiveGeometry &out, uint32 a, uint32 b, uint32 c, uint32 d)
{
	// Two triangles: (a,b,c) and (a,c,d).
	addTriangle(out, a, b, c);
	addTriangle(out, a, c, d);
}

} // anonymous namespace

PrimitiveGeometry cube()
{
	return box(1.0f, 1.0f, 1.0f);
}

PrimitiveGeometry box(float width, float height, float depth)
{
	PrimitiveGeometry out;

	float hw = width * 0.5f;
	float hh = height * 0.5f;
	float hd = depth * 0.5f;

	// 6 faces, each 4 vertices outward-facing CCW.
	struct Face { Vector3 n; Vector3 u; Vector3 v; float extentU; float extentV; };

	Face faces[6] =
	{
		// +X
		{ Vector3( 1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f), hd, hh },
		// -X
		{ Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f,  1.0f), Vector3(0.0f, 1.0f, 0.0f), hd, hh },
		// +Y (u x v must equal the face normal for CCW winding)
		{ Vector3(0.0f,  1.0f, 0.0f), Vector3(0.0f, 0.0f,  1.0f), Vector3(1.0f, 0.0f, 0.0f), hd, hw },
		// -Y
		{ Vector3(0.0f, -1.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f), hd, hw },
		// +Z
		{ Vector3(0.0f, 0.0f,  1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), hw, hh },
		// -Z
		{ Vector3(0.0f, 0.0f, -1.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), hw, hh },
	};

	for (int f = 0; f < 6; f++)
	{
		const Face &face = faces[f];
		Vector3 center = face.n * Vector3(face.n.x != 0.0f ? hw : (face.n.y != 0.0f ? hh : hd));
		center = Vector3(face.n.x * hw, face.n.y * hh, face.n.z * hd);

		uint32 base = (uint32) out.vertices.size();

		for (int corner = 0; corner < 4; corner++)
		{
			float su = (corner == 1 || corner == 2) ? 1.0f : -1.0f;
			float sv = (corner >= 2) ? 1.0f : -1.0f;

			Vector3 p = center + face.u * (su * face.extentU) + face.v * (sv * face.extentV);
			float u = su > 0.0f ? 1.0f : 0.0f;
			float v = sv > 0.0f ? 1.0f : 0.0f;

			addVertex(out, p, face.n, u, v);
		}

		// Corners: (-u,-v), (+u,-v), (+u,+v), (-u,+v) -> quad (0,1,2,3).
		addQuad(out, base + 0, base + 1, base + 2, base + 3);
	}

	return out;
}

PrimitiveGeometry sphere(int latitudes, int longitudes, float radius)
{
	PrimitiveGeometry out;

	latitudes = std::max(2, latitudes);
	longitudes = std::max(3, longitudes);

	// Row-major grid: (latitudes+1) rows of vertices, longitudes columns.
	for (int lat = 0; lat <= latitudes; lat++)
	{
		float theta = float(lat) / float(latitudes) * PI; // 0..PI from bottom pole to top.
		float sinT = sinf(theta);
		float cosT = cosf(theta);

		for (int lon = 0; lon <= longitudes; lon++)
		{
			float phi = float(lon) / float(longitudes) * 2.0f * PI;
			float sinP = sinf(phi);
			float cosP = cosf(phi);

			// Y-up; phi rotates around the Y axis starting at -Z.
			Vector3 n(sinT * cosP, cosT, -sinT * sinP);
			Vector3 p = n * radius;

			float u = float(lon) / float(longitudes);
			float v = float(lat) / float(latitudes);

			addVertex(out, p, n, u, v);
		}
	}

	int stride = longitudes + 1;
	for (int lat = 0; lat < latitudes; lat++)
	{
		for (int lon = 0; lon < longitudes; lon++)
		{
			uint32 i0 = (uint32) (lat * stride + lon);
			uint32 i1 = i0 + 1;
			uint32 i2 = i0 + stride;
			uint32 i3 = i2 + 1;

			// Outward CCW: depends on grid orientation.
			addTriangle(out, i0, i2, i1);
			addTriangle(out, i1, i2, i3);
		}
	}

	return out;
}

PrimitiveGeometry plane(int subdivisions, float size)
{
	PrimitiveGeometry out;

	subdivisions = std::max(1, subdivisions);

	float half = size * 0.5f;

	for (int row = 0; row <= subdivisions; row++)
	{
		for (int col = 0; col <= subdivisions; col++)
		{
			float u = float(col) / float(subdivisions);
			float v = float(row) / float(subdivisions);

			Vector3 p(-half + u * size, 0.0f, -half + v * size);
			Vector3 n(0.0f, 1.0f, 0.0f);

			addVertex(out, p, n, u, v);
		}
	}

	int stride = subdivisions + 1;
	for (int row = 0; row < subdivisions; row++)
	{
		for (int col = 0; col < subdivisions; col++)
		{
			uint32 i0 = (uint32) (row * stride + col);
			uint32 i1 = i0 + 1;
			uint32 i2 = i0 + stride;
			uint32 i3 = i2 + 1;

			// +Y facing plane. Viewed from above with X right / Z down toward
			// viewer ... ensure CCW around the +Y normal:
			addTriangle(out, i0, i2, i1);
			addTriangle(out, i1, i2, i3);
		}
	}

	return out;
}

PrimitiveGeometry cylinder(int slices, float radius, float height)
{
	PrimitiveGeometry out;

	slices = std::max(3, slices);

	float hh = height * 0.5f;

	// Side surface: two rings, y = -hh and +hh.
	for (int ring = 0; ring <= 1; ring++)
	{
		float y = (ring == 0) ? -hh : hh;
		float v = (ring == 0) ? 0.0f : 1.0f;

		for (int slice = 0; slice <= slices; slice++)
		{
			float phi = float(slice) / float(slices) * 2.0f * PI;
			float x = sinf(phi) * radius;
			float z = -cosf(phi) * radius;

			Vector3 n(x / radius, 0.0f, z / radius);

			addVertex(out, Vector3(x, y, z), n, float(slice) / float(slices), v);
		}
	}

	int stridelen = slices + 1;
	for (int slice = 0; slice < slices; slice++)
	{
		uint32 i0 = (uint32) (0 * stridelen + slice);
		uint32 i1 = i0 + 1;
		uint32 i2 = (uint32) (1 * stridelen + slice);
		uint32 i3 = i2 + 1;

		// Outward CCW: slice+1 moves toward +X from -Z, rows go bottom to
		// top (cross-verified at phi=0 where the outward normal is -Z).
		addTriangle(out, i0, i2, i1);
		addTriangle(out, i1, i2, i3);
	}

	// Bottom cap (-Y, normal down).
	{
		uint32 centerbase = (uint32) out.vertices.size();
		addVertex(out, Vector3(0.0f, -hh, 0.0f), Vector3(0.0f, -1.0f, 0.0f), 0.5f, 0.5f);

		uint32 ringbase = (uint32) out.vertices.size();
		for (int slice = 0; slice <= slices; slice++)
		{
			float phi = float(slice) / float(slices) * 2.0f * PI;
			Vector3 p(sinf(phi) * radius, -hh, -cosf(phi) * radius);
			Vector3 n(0.0f, -1.0f, 0.0f);
			float u = 0.5f + 0.5f * (p.x / radius);
			float vv = 0.5f + 0.5f * (-p.z / radius);
			addVertex(out, p, n, u, vv);
		}

		for (int slice = 0; slice < slices; slice++)
			addTriangle(out, centerbase, ringbase + slice, ringbase + slice + 1);
	}

	// Top cap (+Y, normal up).
	{
		uint32 centerbase = (uint32) out.vertices.size();
		addVertex(out, Vector3(0.0f, hh, 0.0f), Vector3(0.0f, 1.0f, 0.0f), 0.5f, 0.5f);

		uint32 ringbase = (uint32) out.vertices.size();
		for (int slice = 0; slice <= slices; slice++)
		{
			float phi = float(slice) / float(slices) * 2.0f * PI;
			Vector3 p(sinf(phi) * radius, hh, -cosf(phi) * radius);
			Vector3 n(0.0f, 1.0f, 0.0f);
			float u = 0.5f + 0.5f * (p.x / radius);
			float vv = 0.5f + 0.5f * (p.z / radius);
			addVertex(out, p, n, u, vv);
		}

		for (int slice = 0; slice < slices; slice++)
			addTriangle(out, centerbase, ringbase + slice + 1, ringbase + slice);
	}

	return out;
}

PrimitiveGeometry cone(int slices, float radius, float height)
{
	PrimitiveGeometry out;

	slices = std::max(3, slices);

	float hh = height * 0.5f;
	float slope = atan2f(radius, height); // half apex angle for smooth-ish normals.

	// Side surface: apex vertex + rim ring (fan).
	uint32 apex = (uint32) out.vertices.size();
	addVertex(out, Vector3(0.0f, hh, 0.0f), Vector3(0.0f, 1.0f, 0.0f), 0.5f, 0.5f);

	uint32 rimbase = (uint32) out.vertices.size();
	for (int slice = 0; slice <= slices; slice++)
	{
		float phi = float(slice) / float(slices) * 2.0f * PI;
		float sinP = sinf(phi);
		float cosP = cosf(phi);

		Vector3 p(sinP * radius, -hh, -cosP * radius);

		// Approximate outward normal, tilted upward by the slope.
		Vector3 n(sinP * cosf(slope), -sinf(slope), -cosP * cosf(slope));

		addVertex(out, p, n, float(slice) / float(slices), 0.0f);
	}

	// Outward CCW fan: rim vertices advance toward +X from -Z, so the
	// triangle (apex, rim[s+1], rim[s]) faces outward.
	for (int slice = 0; slice < slices; slice++)
		addTriangle(out, apex, rimbase + slice + 1, rimbase + slice);

	// Bottom cap (-Y).
	{
		uint32 centerbase = (uint32) out.vertices.size();
		addVertex(out, Vector3(0.0f, -hh, 0.0f), Vector3(0.0f, -1.0f, 0.0f), 0.5f, 0.5f);

		uint32 capbase = (uint32) out.vertices.size();
		for (int slice = 0; slice <= slices; slice++)
		{
			float phi = float(slice) / float(slices) * 2.0f * PI;
			Vector3 p(sinf(phi) * radius, -hh, -cosf(phi) * radius);
			addVertex(out, p, Vector3(0.0f, -1.0f, 0.0f), 0.5f + 0.5f * (p.x / radius), 0.5f + 0.5f * (-p.z / radius));
		}

		for (int slice = 0; slice < slices; slice++)
			addTriangle(out, centerbase, capbase + slice, capbase + slice + 1);
	}

	return out;
}

PrimitiveGeometry capsule(int slices, int rings, float radius, float height)
{
	PrimitiveGeometry out;

	slices = std::max(3, slices);
	rings = std::max(1, rings);

	// "height" is the total height including the hemisphere caps. The
	// cylindrical (barrel) section gets the remainder.
	float halfHeight = height * 0.5f;
	float halfCylinder = std::max(0.0f, halfHeight - radius);

	Vector3 bottomCenter(0.0f, -halfCylinder, 0.0f);
	Vector3 topCenter(0.0f, halfCylinder, 0.0f);

	// Build a ring list: (normal's cos/sin theta, y position, barrel flag).
	struct Row
	{
		float theta;     // angle from +Y of the outward normal direction.
		float y;
		bool isBarrel;   // barrel rows have horizontal-only normals.
	};

	std::vector<Row> rows;

	// Bottom hemisphere: theta from PI (bottom pole) to PI/2 (equator).
	for (int i = 0; i <= rings; i++)
	{
		float theta = PI - (float(i) / float(rings)) * (PI * 0.5f);
		float y = bottomCenter.y + radius * cosf(theta);
		rows.push_back({ theta, y, false });
	}

	// Single barrel top row connecting both hemispheres at +y.
	rows.push_back({ PI * 0.5f, topCenter.y, true });

	// Top hemisphere: theta from just below PI/2 down to 0 (top pole).
	for (int i = 1; i <= rings; i++)
	{
		float theta = (PI * 0.5f) * (1.0f - float(i) / float(rings));
		float y = topCenter.y + radius * cosf(theta);
		rows.push_back({ theta, y, false });
	}

	size_t rowCount = rows.size();

	for (size_t r = 0; r < rowCount; r++)
	{
		const Row &row = rows[r];

		float sinT = sinf(row.theta);
		float cosT = cosf(row.theta);

		for (int slice = 0; slice <= slices; slice++)
		{
			float phi = float(slice) / float(slices) * 2.0f * PI;
			float sinP = sinf(phi);
			float cosP = cosf(phi);

			// Y-up. phi starts at -Z and rotates toward +X.
			Vector3 n(sinT * cosP, cosT, -sinT * sinP);

			if (row.isBarrel)
				n = Vector3(n.x, 0.0f, n.z);

			Vector3 p(sinf(row.theta) * radius * cosP, row.y, -sinf(row.theta) * radius * sinP);

			float u = float(slice) / float(slices);
			float v = float(r) / float(rowCount - 1);

			addVertex(out, p, n, u, v);
		}
	}

	int stridelen = slices + 1;
	for (size_t r = 0; r + 1 < rowCount; r++)
	{
		for (int slice = 0; slice < slices; slice++)
		{
			uint32 i0 = (uint32) (r * stridelen + slice);
			uint32 i1 = i0 + 1;
			uint32 i2 = (uint32) ((r + 1) * stridelen + slice);
			uint32 i3 = i2 + 1;

			// Outward CCW (rows run bottom -> top).
			addTriangle(out, i0, i1, i2);
			addTriangle(out, i1, i3, i2);
		}
	}

	return out;
}

PrimitiveGeometry torus(int segments, int rings, float majorRadius, float minorRadius)
{
	PrimitiveGeometry out;

	segments = std::max(3, segments);
	rings = std::max(3, rings);

	for (int seg = 0; seg <= segments; seg++)
	{
		float u = float(seg) / float(segments) * 2.0f * PI;
		float cosU = cosf(u);
		float sinU = sinf(u);

		for (int ringr = 0; ringr <= rings; ringr++)
		{
			float v = float(ringr) / float(rings) * 2.0f * PI;
			float cosV = cosf(v);
			float sinV = sinf(v);

			// Major circle in the XZ plane (starting from +X), tube rotating
			// around it. Y stays up.
			Vector3 center(cosU * majorRadius, 0.0f, sinU * majorRadius);

			// Outward-pointing tube normal.
			Vector3 n(cosU * cosV, sinV, sinU * cosV);

			Vector3 p = center + n * minorRadius;

			addVertex(out, p, n, float(seg) / float(segments), float(ringr) / float(rings));
		}
	}

	int stridelen = rings + 1;
	for (int seg = 0; seg < segments; seg++)
	{
		for (int ringr = 0; ringr < rings; ringr++)
		{
			uint32 i0 = (uint32) (seg * stridelen + ringr);
			uint32 i1 = i0 + 1;
			uint32 i2 = (uint32) ((seg + 1) * stridelen + ringr);
			uint32 i3 = i2 + 1;

			// Outward CCW: +ringr runs "up" the tube, +seg runs around the
			// major circle (cross-verified at u=0, v=0 where the outward
			// normal is +X).
			addTriangle(out, i0, i1, i2);
			addTriangle(out, i1, i3, i2);
		}
	}

	return out;
}

} // primitives

} // graphics3d
} // love
