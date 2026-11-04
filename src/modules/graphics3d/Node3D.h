/**
 * LOVING3D - scene graph nodes for the LOVE game engine.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org).
 * The underlying engine remains freely distributable under the terms of
 * its original zlib license (see license.txt).
 **/

#pragma once

#include "Transform3D.h"
#include "Mesh3D.h"
#include "Model3D.h"
#include "common/Object.h"

#include <vector>

namespace love
{
namespace graphics3d
{

class Node3D : public love::Object
{
public:
	static love::Type type;

	Node3D();
	virtual ~Node3D();

	void setParent(Node3D *parent);
	Node3D *getParent() const { return parent; }
	void addChild(Node3D *child);
	void removeChild(Node3D *child);
	size_t getChildCount() const { return children.size(); }
	Node3D *getChild(size_t index) const;

	void setPosition(float x, float y, float z) { transform.setPosition(x, y, z); }
	void setRotation(float x, float y, float z) { transform.setRotationEulerDegrees(Vector3(x, y, z)); }
	void setScale(float x, float y, float z) { transform.setScale(Vector3(x, y, z)); }
	void setMesh(Mesh3D *mesh);
	void setModel(Model3D *model);

	Matrix4 getLocalMatrix() const { return transform.getLocalMatrix(); }
	Matrix4 getWorldMatrix() const;
	Vector3 getWorldPosition() const { return getWorldMatrix() * Vector3(); }
	Mesh3D *getMesh() const { return mesh.get(); }
	Model3D *getModel() const { return model.get(); }

private:
	Node3D *parent = nullptr;
	std::vector<StrongRef<Node3D>> children;
	Transform3D transform;
	StrongRef<Mesh3D> mesh;
	StrongRef<Model3D> model;
};

} // graphics3d
} // love
