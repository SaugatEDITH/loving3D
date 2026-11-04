/**
 * LOVING3D - Lua bindings for the 3D module.
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

#ifndef LOVING3D_WRAP_GRAPHICS3D_H
#define LOVING3D_WRAP_GRAPHICS3D_H

// LOVE
#include "common/runtime.h"
#include "Graphics3D.h"

namespace love
{
namespace graphics3d
{

extern "C" LOVE_EXPORT int luaopen_love_graphics3d(lua_State *L);

} // graphics3d
} // love

#endif // LOVING3D_WRAP_GRAPHICS3D_H
