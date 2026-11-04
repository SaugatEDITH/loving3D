/**
 * LOVING3D - primitive shape generators for the LOVE game engine.
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
 * Procedural geometry generators for 3D primitives.
 *
 * All generators produce a Vertex3D vertex list plus a uint32 index list.
 * Winding is counter-clockwise when seen from outside the shape (front
 * faces point outward), under LOVING3D's right-handed coordinate system:
 * X is right, Y is up, -Z is "into the screen" for a default camera.
 *
 * Vertex colors default to white; texcoords are simple parameterizations.
 **/

#pragma once

#include "Mesh3D.h"

#include <vector>

namespace love
{
namespace graphics3d
{

struct PrimitiveGeometry
{
	std::vector<Vertex3D> vertices;
	std::vector<uint32> indices;
};

namespace primitives
{

/** A 1x1x1 cube centered at the origin. Box scales each axis extent. **/
PrimitiveGeometry cube();
PrimitiveGeometry box(float width, float height, float depth);

/**
 * Sphere around the origin with radius 1. latitudes/longitudes control
 * tessellation (pure UV sphere).
 **/
PrimitiveGeometry sphere(int latitudes = 16, int longitudes = 24, float radius = 1.0f);

/**
 * Plane on the XZ plane facing +Y (normal points "up"), size 1x1 around the
 * origin. Useful as ground. subdivisions adds extra tessellation.
 **/
PrimitiveGeometry plane(int subdivisions = 1, float size = 1.0f);

/** Cylinder aligned with the Y axis, radius 1, height 1, centered. **/
PrimitiveGeometry cylinder(int slices = 24, float radius = 1.0f, float height = 1.0f);

/** Cone aligned with the Y axis: apex at +height/2, disc base at -height/2. **/
PrimitiveGeometry cone(int slices = 24, float radius = 1.0f, float height = 1.0f);

/** Capsule aligned with the Y axis: cylinder in the middle, hemisphere ends. **/
PrimitiveGeometry capsule(int slices = 24, int rings = 8, float radius = 0.5f, float height = 1.0f);

/** Torus in the XZ plane, with the tube rotating around the Y axis. **/
PrimitiveGeometry torus(int segments = 32, int rings = 16, float majorRadius = 1.0f, float minorRadius = 0.3f);

} // primitives

} // graphics3d
} // love
