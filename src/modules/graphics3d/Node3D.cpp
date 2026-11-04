/**
 * LOVING3D - scene graph node implementation.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org).
 * The underlying engine remains freely distributable under the terms of
 * its original zlib license (see license.txt).
 **/

#include "Node3D.h"
#include "common/Exception.h"

namespace love
{
namespace graphics3d
{

love::Type Node3D::type("Node3D", &Object::type);

Node3D::Node3D() = default;

Node3D::~Node3D()
{
	if (parent != nullptr)
		parent->removeChild(this);

	for (StrongRef<Node3D> &child : children)
		child->parent = nullptr;
}

void Node3D::setParent(Node3D *newParent)
{
	if (newParent == this)
		throw love::Exception("A Node3D cannot be its own parent.");

	for (Node3D *ancestor = newParent; ancestor != nullptr; ancestor = ancestor->parent)
	{
		if (ancestor == this)
			throw love::Exception("Setting this parent would create a Node3D cycle.");
	}

	if (parent == newParent)
		return;

	if (parent != nullptr)
		parent->removeChild(this);

	if (newParent != nullptr)
	{
		parent = newParent;
		newParent->children.emplace_back(this);
	}
}

void Node3D::addChild(Node3D *child)
{
	if (child == nullptr)
		throw love::Exception("Expected a Node3D child.");
	child->setParent(this);
}

void Node3D::removeChild(Node3D *child)
{
	for (auto it = children.begin(); it != children.end(); ++it)
	{
		if (it->get() == child)
		{
			children.erase(it);
			child->parent = nullptr;
			return;
		}
	}
}

Node3D *Node3D::getChild(size_t index) const
{
	if (index >= children.size())
		throw love::Exception("Node3D child index is out of range.");
	return children[index].get();
}

void Node3D::setMesh(Mesh3D *mesh_)
{
	mesh.set(mesh_);
	model.set(nullptr);
}

void Node3D::setModel(Model3D *model_)
{
	model.set(model_);
	mesh.set(nullptr);
}

Matrix4 Node3D::getWorldMatrix() const
{
	Matrix4 local = transform.getLocalMatrix();
	return parent != nullptr ? parent->getWorldMatrix() * local : local;
}

} // graphics3d
} // love
