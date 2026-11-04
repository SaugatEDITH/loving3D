/**
 * LOVING3D - model implementation.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org). The underlying engine
 * remains freely distributable under the terms of its original zlib license
 * (see license.txt).
 **/

#include "Model3D.h"

#include "common/Exception.h"

#include <algorithm>

namespace love
{
namespace graphics3d
{

love::Type Model3D::type("Model3D", &Object::type);

Model3D::Model3D()
{
}

Model3D::~Model3D()
{
}

size_t Model3D::addPart(Mesh3D *mesh, Material3D *materialOverride, const Matrix4 &localMatrix)
{
	if (mesh == nullptr)
		throw love::Exception("Expected a Mesh3D argument.");

	Part part;
	part.mesh.set(mesh);
	part.material.set(materialOverride != nullptr ? materialOverride : mesh->getMaterial());
	part.localMatrix = localMatrix;

	parts.push_back(part);
	return parts.size() - 1;
}

void Model3D::removePart(size_t index)
{
	if (index >= parts.size())
		throw love::Exception("Part index out of range.");

	parts.erase(parts.begin() + index);
}

Mesh3D *Model3D::getPartMesh(size_t index) const
{
	if (index >= parts.size())
		return nullptr;

	return parts[index].mesh.get();
}

Material3D *Model3D::getPartMaterial(size_t index) const
{
	if (index >= parts.size())
		return nullptr;

	return parts[index].material.get();
}

const Matrix4 &Model3D::getPartMatrix(size_t index) const
{
	static const Matrix4 identity = Matrix4::identity();

	if (index >= parts.size())
		return identity;

	return parts[index].localMatrix;
}

bool Model3D::getBoundingBox(Vector3 &minBounds, Vector3 &maxBounds) const
{
	bool hasbbox = false;

	for (const Part &part : parts)
	{
		Vector3 minv, maxv;
		if (part.mesh.get() == nullptr || !part.mesh.get()->getBoundingBox(minv, maxv))
			continue;

		// 8 corners of the part box transformed by the part's local matrix.
		for (int corner = 0; corner < 8; corner++)
		{
			Vector3 p(
				(corner & 1) ? maxv.x : minv.x,
				(corner & 2) ? maxv.y : minv.y,
				(corner & 4) ? maxv.z : minv.z);

			p = part.localMatrix * p;

			if (!hasbbox)
			{
				minBounds = p;
				maxBounds = p;
				hasbbox = true;
			}
			else
			{
				minBounds.x = std::min(minBounds.x, p.x);
				minBounds.y = std::min(minBounds.y, p.y);
				minBounds.z = std::min(minBounds.z, p.z);
				maxBounds.x = std::max(maxBounds.x, p.x);
				maxBounds.y = std::max(maxBounds.y, p.y);
				maxBounds.z = std::max(maxBounds.z, p.z);
			}
		}
	}

	return hasbbox;
}

} // graphics3d
} // love
