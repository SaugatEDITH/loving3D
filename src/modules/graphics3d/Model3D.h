/**
 * LOVING3D - models / part containers for the LOVE game engine.
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
 * A Model3D groups one or more Mesh3D "parts", each with its own material
 * and optional local transform. Used for imported multi-material assets
 * (e.g. OBJ files with several groups), and as the long-term home for
 * skeletal animation and skinning data.
 **/

#pragma once

#include "Mesh3D.h"

#include "common/Object.h"

#include <vector>

namespace love
{
namespace graphics3d
{

class Model3D : public love::Object
{
public:

	static love::Type type;

	Model3D();
	virtual ~Model3D();

	/**
	 * Adds a Mesh3D part. materialOverride can be null to use the mesh's
	 * own material. localMatrix positions the part inside the model (zero
	 * offset is the identity).
	 **/
	size_t addPart(Mesh3D *mesh, Material3D *materialOverride, const Matrix4 &localMatrix);
	void removePart(size_t index);
	size_t getPartCount() const { return parts.size(); }

	// Part accessors used by the 3D render pass.
	Mesh3D *getPartMesh(size_t index) const;
	Material3D *getPartMaterial(size_t index) const;
	const Matrix4 &getPartMatrix(size_t index) const;

	/**
	 * Bounding box in model local space (union of all parts). Returns false
	 * when the model has no parts with vertices.
	 **/
	bool getBoundingBox(Vector3 &minBounds, Vector3 &maxBounds) const;
	bool getBounds(Vector3 &minBounds, Vector3 &maxBounds) const { return getBoundingBox(minBounds, maxBounds); }

private:

	friend class Graphics3D;

	struct Part
	{
		StrongRef<Mesh3D> mesh;
		StrongRef<Material3D> material;
		Matrix4 localMatrix;
	};

	std::vector<Part> parts;

}; // Model3D

} // graphics3d
} // love
