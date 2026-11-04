/**
 * LOVING3D - glTF 2.0 / GLB model importer.
 *
 * Copyright (c) 2026 Saugat Pokharel
 *
 * This file is part of LOVING3D, an independent extension adding native 3D
 * capabilities to the LOVE game engine (love2d.org). The underlying engine
 * remains freely distributable under the terms of its original zlib license
 * (see license.txt).
 **/

#pragma once

#include "Model3D.h"
#include "modules/graphics/Texture.h"

#include <functional>
#include <string>
#include <vector>

namespace love
{
namespace graphics
{
class Graphics;
}

namespace graphics3d
{

class GLTFLoader
{
public:
	using ReadFile = std::function<std::vector<uint8>(const std::string &)>;
	using LoadTexture = std::function<love::graphics::Texture *(
		const std::vector<uint8> &, const love::graphics::SamplerState &, bool)>;

	static Model3D *load(const std::string &path, love::graphics::Graphics *graphics,
		const ReadFile &readFile, const LoadTexture &loadTexture);
};

} // graphics3d
} // love
