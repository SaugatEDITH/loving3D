/**
 * LOVING3D - Lua bindings implementation for the 3D module.
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

#include "wrap_Graphics3D.h"

#include "common/config.h"
#include "common/Exception.h"
#include "common/StringMap.h"

#include "modules/graphics/Texture.h"
#include "modules/graphics/wrap_Texture.h"
#include "modules/filesystem/Filesystem.h"
#include "modules/filesystem/FileData.h"
#include "modules/image/Image.h"
#include "modules/image/ImageData.h"
#include "GLTFLoader.h"
#include "Node3D.h"
#include "data/ByteData.h"

#include <cmath>
#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace love
{
namespace graphics3d
{

#define instance() (Module::getInstance<Graphics3D>(Module::M_GRAPHICS3D))

// Type registration openers (defined at the bottom of this file).
static int camera3d_opentype(lua_State *L);
static int material3d_opentype(lua_State *L);
static int light3d_opentype(lua_State *L);
static int mesh3d_opentype(lua_State *L);
static int model3d_opentype(lua_State *L);
static int node3d_opentype(lua_State *L);

/**
 * String maps for the module's enums. These use LOVE's STRINGMAP helper,
 * generating getConstant / getConstants pairs.
 **/

static const char *lightTypeStrings[] =
{
	"directional",
	"point",
	"spot",
};

static bool lightTypeFromString(const char *in, Light3D::LightType &out)
{
	for (int i = 0; i < Light3D::LIGHT_MAX_ENUM; i++)
	{
		if (strcmp(lightTypeStrings[i], in) == 0)
		{
			out = (Light3D::LightType) i;
			return true;
		}
	}

	return false;
}

static const char *getLightTypeString(Light3D::LightType type)
{
	return lightTypeStrings[(int) type];
}

static const char *projectionModeStrings[] =
{
	"perspective",
	"orthographic",
};

static bool projectionModeFromString(const char *in, Camera3D::ProjectionMode &out)
{
	for (int i = 0; i < Camera3D::PROJECTION_MAX_ENUM; i++)
	{
		if (strcmp(projectionModeStrings[i], in) == 0)
		{
			out = (Camera3D::ProjectionMode) i;
			return true;
		}
	}

	return false;
}

static const char *getProjectionModeString(Camera3D::ProjectionMode mode)
{
	return projectionModeStrings[(int) mode];
}

/**
 * Cameras.
 **/

Camera3D *luax_checkcamera3d(lua_State *L, int idx)
{
	return luax_checktype<Camera3D>(L, idx);
}

static int w_Camera3D_setPosition(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	t->setPosition((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3), (float) luaL_checknumber(L, 4));
	return 0;
}

static int w_Camera3D_getPosition(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	const Vector3 &pos = t->getPosition();
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	return 3;
}

static int w_Camera3D_setRotation(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	t->setRotation((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3), (float) luaL_checknumber(L, 4));
	return 0;
}

static int w_Camera3D_getRotation(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	Vector3 rot = t->getRotationDegrees();
	lua_pushnumber(L, rot.x);
	lua_pushnumber(L, rot.y);
	lua_pushnumber(L, rot.z);
	return 3;
}

static int w_Camera3D_lookAt(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	luax_catchexcept(L, [&]() { t->lookAt((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3), (float) luaL_checknumber(L, 4)); });
	return 0;
}

static int w_Camera3D_setFOV(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	luax_catchexcept(L, [&]() { t->setFOV((float) luaL_checknumber(L, 2)); });
	return 0;
}

static int w_Camera3D_getFOV(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	lua_pushnumber(L, t->getFOV());
	return 1;
}

static int w_Camera3D_setNear(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	luax_catchexcept(L, [&]() { t->setNear((float) luaL_checknumber(L, 2)); });
	return 0;
}

static int w_Camera3D_getNear(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	lua_pushnumber(L, t->getNear());
	return 1;
}

static int w_Camera3D_setFar(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	luax_catchexcept(L, [&]() { t->setFar((float) luaL_checknumber(L, 2)); });
	return 0;
}

static int w_Camera3D_getFar(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	lua_pushnumber(L, t->getFar());
	return 1;
}

static int w_Camera3D_setAspect(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	luax_catchexcept(L, [&]() { t->setAspect((float) luaL_checknumber(L, 2)); });
	return 0;
}

static int w_Camera3D_getAspect(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	lua_pushnumber(L, t->getAspect());
	return 1;
}

static int w_Camera3D_setProjectionMode(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	const char *str = luaL_checkstring(L, 2);
	Camera3D::ProjectionMode mode;

	if (!projectionModeFromString(str, mode))
		return luax_enumerror(L, "projection mode", str);

	t->setProjectionMode(mode);
	return 0;
}

static int w_Camera3D_getProjectionMode(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	lua_pushstring(L, getProjectionModeString(t->getProjectionMode()));
	return 1;
}

static int w_Camera3D_setOrthoExtents(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	float left = (float) luaL_checknumber(L, 2);
	float right = (float) luaL_checknumber(L, 3);
	float bottom = (float) luaL_checknumber(L, 4);
	float top = (float) luaL_checknumber(L, 5);
	luax_catchexcept(L, [&]() { t->setOrthoExtents(left, right, bottom, top); });
	return 0;
}

static int w_Camera3D_getOrthoExtents(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	float left, right, bottom, top;
	t->getOrthoExtents(left, right, bottom, top);
	lua_pushnumber(L, left);
	lua_pushnumber(L, right);
	lua_pushnumber(L, bottom);
	lua_pushnumber(L, top);
	return 4;
}

static int w_Camera3D_worldToScreen(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	float width = (float) luaL_optnumber(L, 5, Module::getInstance<love::graphics::Graphics>(Module::M_GRAPHICS)->getWidth());
	float height = (float) luaL_optnumber(L, 6, Module::getInstance<love::graphics::Graphics>(Module::M_GRAPHICS)->getHeight());
	Vector3 screen;
	luax_catchexcept(L, [&]()
	{
		screen = t->worldToScreen(Vector3(
			(float) luaL_checknumber(L, 2),
			(float) luaL_checknumber(L, 3),
			(float) luaL_checknumber(L, 4)), width, height);
	});
	lua_pushnumber(L, screen.x);
	lua_pushnumber(L, screen.y);
	lua_pushnumber(L, screen.z);
	return 3;
}

static int w_Camera3D_screenToWorld(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	float width = (float) luaL_optnumber(L, 5, Module::getInstance<love::graphics::Graphics>(Module::M_GRAPHICS)->getWidth());
	float height = (float) luaL_optnumber(L, 6, Module::getInstance<love::graphics::Graphics>(Module::M_GRAPHICS)->getHeight());
	Vector3 world;
	luax_catchexcept(L, [&]()
	{
		world = t->screenToWorld(
			(float) luaL_checknumber(L, 2),
			(float) luaL_checknumber(L, 3),
			(float) luaL_checknumber(L, 4), width, height);
	});
	lua_pushnumber(L, world.x);
	lua_pushnumber(L, world.y);
	lua_pushnumber(L, world.z);
	return 3;
}

static int w_Camera3D_screenRay(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	float width = (float) luaL_optnumber(L, 4, Module::getInstance<love::graphics::Graphics>(Module::M_GRAPHICS)->getWidth());
	float height = (float) luaL_optnumber(L, 5, Module::getInstance<love::graphics::Graphics>(Module::M_GRAPHICS)->getHeight());
	Vector3 origin, direction;
	luax_catchexcept(L, [&]()
	{
		t->screenRay((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3),
			width, height, origin, direction);
	});
	lua_pushnumber(L, origin.x);
	lua_pushnumber(L, origin.y);
	lua_pushnumber(L, origin.z);
	lua_pushnumber(L, direction.x);
	lua_pushnumber(L, direction.y);
	lua_pushnumber(L, direction.z);
	return 6;
}

static int w_Camera3D_getForward(lua_State *L)
{
	Vector3 direction = luax_checkcamera3d(L, 1)->getForward();
	lua_pushnumber(L, direction.x);
	lua_pushnumber(L, direction.y);
	lua_pushnumber(L, direction.z);
	return 3;
}

static int w_Camera3D_getRight(lua_State *L)
{
	Vector3 direction = luax_checkcamera3d(L, 1)->getRight();
	lua_pushnumber(L, direction.x);
	lua_pushnumber(L, direction.y);
	lua_pushnumber(L, direction.z);
	return 3;
}

static int w_Camera3D_getUp(lua_State *L)
{
	Vector3 direction = luax_checkcamera3d(L, 1)->getUp();
	lua_pushnumber(L, direction.x);
	lua_pushnumber(L, direction.y);
	lua_pushnumber(L, direction.z);
	return 3;
}

static int w_Camera3D_moveRelative(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	t->moveRelative((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3), (float) luaL_checknumber(L, 4));
	return 0;
}

static int w_Camera3D_rotateYaw(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	t->rotateYaw((float) luaL_checknumber(L, 2));
	return 0;
}

static int w_Camera3D_rotatePitch(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	t->rotatePitch((float) luaL_checknumber(L, 2));
	return 0;
}

static int w_Camera3D_rotateRoll(lua_State *L)
{
	Camera3D *t = luax_checkcamera3d(L, 1);
	t->rotateRoll((float) luaL_checknumber(L, 2));
	return 0;
}

static const luaL_Reg w_Camera3D_functions[] =
{
	{ "setPosition", w_Camera3D_setPosition },
	{ "getPosition", w_Camera3D_getPosition },
	{ "setRotation", w_Camera3D_setRotation },
	{ "getRotation", w_Camera3D_getRotation },
	{ "lookAt", w_Camera3D_lookAt },
	{ "setFOV", w_Camera3D_setFOV },
	{ "getFOV", w_Camera3D_getFOV },
	{ "setNear", w_Camera3D_setNear },
	{ "getNear", w_Camera3D_getNear },
	{ "setFar", w_Camera3D_setFar },
	{ "getFar", w_Camera3D_getFar },
	{ "setAspect", w_Camera3D_setAspect },
	{ "getAspect", w_Camera3D_getAspect },
	{ "setProjectionMode", w_Camera3D_setProjectionMode },
	{ "getProjectionMode", w_Camera3D_getProjectionMode },
	{ "setOrthoExtents", w_Camera3D_setOrthoExtents },
	{ "getOrthoExtents", w_Camera3D_getOrthoExtents },
	{ "worldToScreen", w_Camera3D_worldToScreen },
	{ "screenToWorld", w_Camera3D_screenToWorld },
	{ "screenRay", w_Camera3D_screenRay },
	{ "getForward", w_Camera3D_getForward },
	{ "getRight", w_Camera3D_getRight },
	{ "getUp", w_Camera3D_getUp },
	{ "moveRelative", w_Camera3D_moveRelative },
	{ "rotateYaw", w_Camera3D_rotateYaw },
	{ "rotatePitch", w_Camera3D_rotatePitch },
	{ "rotateRoll", w_Camera3D_rotateRoll },
	{ 0, 0 }
};

/**
 * Materials.
 **/

Material3D *luax_checkmaterial3d(lua_State *L, int idx)
{
	return luax_checktype<Material3D>(L, idx);
}

static int w_Material3D_setColor(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	float r = (float) luaL_checknumber(L, 2);
	float g = (float) luaL_checknumber(L, 3);
	float b = (float) luaL_checknumber(L, 4);
	float a = (float) luaL_optnumber(L, 5, 1.0);
	t->setColor(r, g, b, a);
	return 0;
}

static int w_Material3D_getColor(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	Vector4 c = t->getColor();
	lua_pushnumber(L, c.x);
	lua_pushnumber(L, c.y);
	lua_pushnumber(L, c.z);
	lua_pushnumber(L, c.w);
	return 4;
}

static int w_Material3D_setDiffuseTexture(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);

	if (lua_isnoneornil(L, 2))
	{
		t->setDiffuseTexture(nullptr);
		return 0;
	}

	love::graphics::Texture *texture = luax_checktype<love::graphics::Texture>(L, 2);
	t->setDiffuseTexture(texture);
	return 0;
}

static int w_Material3D_getDiffuseTexture(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	auto texture = t->getDiffuseTexture();
	if (!texture)
		return 0;

	luax_pushtype(L, texture);
	return 1;
}

static int w_Material3D_setNormalTexture(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);

	if (lua_isnoneornil(L, 2))
	{
		t->setNormalTexture(nullptr);
		return 0;
	}

	love::graphics::Texture *texture = luax_checktype<love::graphics::Texture>(L, 2);
	t->setNormalTexture(texture);
	return 0;
}

static int w_Material3D_getNormalTexture(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	auto texture = t->getNormalTexture();
	if (!texture)
		return 0;

	luax_pushtype(L, texture);
	return 1;
}

static int w_Material3D_setSpecularTexture(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);

	if (lua_isnoneornil(L, 2))
	{
		t->setSpecularTexture(nullptr);
		return 0;
	}

	love::graphics::Texture *texture = luax_checktype<love::graphics::Texture>(L, 2);
	t->setSpecularTexture(texture);
	return 0;
}

static int w_Material3D_getSpecularTexture(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	auto texture = t->getSpecularTexture();
	if (!texture)
		return 0;

	luax_pushtype(L, texture);
	return 1;
}

static int w_Material3D_setEmissiveTexture(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);

	if (lua_isnoneornil(L, 2))
	{
		t->setEmissiveTexture(nullptr);
		return 0;
	}

	love::graphics::Texture *texture = luax_checktype<love::graphics::Texture>(L, 2);
	t->setEmissiveTexture(texture);
	return 0;
}

static int w_Material3D_getEmissiveTexture(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	auto texture = t->getEmissiveTexture();
	if (!texture)
		return 0;

	luax_pushtype(L, texture);
	return 1;
}

static int w_Material3D_setSpecularParams(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	float strength = (float) luaL_checknumber(L, 2);
	float shininess = (float) luaL_checknumber(L, 3);
	t->setSpecularParams(strength, shininess);
	return 0;
}

static int w_Material3D_getSpecularParams(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	float strength, shininess;
	t->getSpecularParams(strength, shininess);
	lua_pushnumber(L, strength);
	lua_pushnumber(L, shininess);
	return 2;
}

static int w_Material3D_setEmissiveColor(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	t->setEmissiveColor(Vector3((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3), (float) luaL_checknumber(L, 4)));
	return 0;
}

static int w_Material3D_getEmissiveColor(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	Vector3 c = t->getEmissiveColor();
	lua_pushnumber(L, c.x);
	lua_pushnumber(L, c.y);
	lua_pushnumber(L, c.z);
	return 3;
}

static int w_Material3D_setUnlit(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	t->setUnlit(luax_checkboolean(L, 2));
	return 0;
}

static int w_Material3D_isUnlit(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	lua_pushboolean(L, t->isUnlit() ? 1 : 0);
	return 1;
}

static int w_Material3D_setTransparent(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	t->setTransparent(luax_checkboolean(L, 2));
	return 0;
}

static int w_Material3D_isTransparent(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	lua_pushboolean(L, t->isTransparent() ? 1 : 0);
	return 1;
}

static int w_Material3D_setAlphaMode(lua_State *L)
{
	Material3D *t = luax_checktype<Material3D>(L, 1);
	const char *mode = luaL_checkstring(L, 2);
	Material3D::AlphaMode alphaMode;
	if (strcmp(mode, "auto") == 0) alphaMode = Material3D::ALPHA_AUTO;
	else if (strcmp(mode, "opaque") == 0) alphaMode = Material3D::ALPHA_OPAQUE;
	else if (strcmp(mode, "mask") == 0) alphaMode = Material3D::ALPHA_MASK;
	else if (strcmp(mode, "blend") == 0) alphaMode = Material3D::ALPHA_BLEND;
	else return luaL_error(L, "Invalid alpha mode '%s' (expected auto, opaque, mask, or blend).", mode);
	t->setAlphaMode(alphaMode);
	return 0;
}

static int w_Material3D_setAlphaCutoff(lua_State *L)
{
	Material3D *t = luax_checktype<Material3D>(L, 1);
	luax_catchexcept(L, [&]() { t->setAlphaCutoff((float) luaL_checknumber(L, 2)); });
	return 0;
}

static int w_Material3D_setAdditiveBlend(lua_State *L)
{
	Material3D *t = luax_checktype<Material3D>(L, 1);
	t->setAdditiveBlend(luax_checkboolean(L, 2));
	return 0;
}

static int w_Material3D_setFragmentFunction(lua_State *L)
{
	Material3D *material = luax_checkmaterial3d(L, 1);
	const char *source = luaL_checkstring(L, 2);
	luax_catchexcept(L, [&]() { instance()->setMaterialFragmentFunction(material, source); });
	return 0;
}

static int w_Material3D_setShader(lua_State *L)
{
	Material3D *material = luax_checkmaterial3d(L, 1);
	love::graphics::Shader *shader = lua_isnoneornil(L, 2)
		? nullptr
		: luax_checktype<love::graphics::Shader>(L, 2);
	material->setShader(shader);
	return 0;
}

static int w_Material3D_getShader(lua_State *L)
{
	Material3D *material = luax_checkmaterial3d(L, 1);
	luax_pushtype(L, material->getShader());
	return 1;
}

static int w_Material3D_reloadShader(lua_State *L)
{
	Material3D *material = luax_checkmaterial3d(L, 1);
	const char *source = luaL_optstring(L, 2, material->getFragmentFunction().c_str());
	if (source[0] == '\0')
		return luaL_error(L, "reloadShader requires a fragment function source or a previously set fragment function.");
	luax_catchexcept(L, [&]() { instance()->setMaterialFragmentFunction(material, source); });
	return 0;
}

static int w_Material3D_setCullMode(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);
	const char *str = luaL_checkstring(L, 2);
	love::graphics::CullMode mode;

	if (!love::graphics::getConstant(str, mode))
		return luax_enumerror(L, "cull mode", love::graphics::getConstants(love::graphics::CULL_BACK), str);

	t->setCullMode(mode);
	return 0;
}

static int w_Material3D_getCullMode(lua_State *L)
{
	Material3D *t = luax_checkmaterial3d(L, 1);

	const char *modestr;
	if (!love::graphics::getConstant(t->getCullMode(), modestr))
		return luaL_error(L, "Unknown cull mode.");

	lua_pushstring(L, modestr);
	return 1;
}

static const luaL_Reg w_Material3D_functions[] =
{
	{ "setColor", w_Material3D_setColor },
	{ "getColor", w_Material3D_getColor },
	{ "setDiffuseTexture", w_Material3D_setDiffuseTexture },
	{ "getDiffuseTexture", w_Material3D_getDiffuseTexture },
	{ "setNormalTexture", w_Material3D_setNormalTexture },
	{ "getNormalTexture", w_Material3D_getNormalTexture },
	{ "setSpecularTexture", w_Material3D_setSpecularTexture },
	{ "getSpecularTexture", w_Material3D_getSpecularTexture },
	{ "setEmissiveTexture", w_Material3D_setEmissiveTexture },
	{ "getEmissiveTexture", w_Material3D_getEmissiveTexture },
	{ "setSpecularParams", w_Material3D_setSpecularParams },
	{ "getSpecularParams", w_Material3D_getSpecularParams },
	{ "setEmissiveColor", w_Material3D_setEmissiveColor },
	{ "getEmissiveColor", w_Material3D_getEmissiveColor },
	{ "setUnlit", w_Material3D_setUnlit },
	{ "isUnlit", w_Material3D_isUnlit },
	{ "setTransparent", w_Material3D_setTransparent },
	{ "isTransparent", w_Material3D_isTransparent },
	{ "setAlphaMode", w_Material3D_setAlphaMode },
	{ "setAlphaCutoff", w_Material3D_setAlphaCutoff },
	{ "setAdditiveBlend", w_Material3D_setAdditiveBlend },
	{ "setShader", w_Material3D_setShader },
	{ "getShader", w_Material3D_getShader },
	{ "setFragmentFunction", w_Material3D_setFragmentFunction },
	{ "reloadShader", w_Material3D_reloadShader },
	{ "setCullMode", w_Material3D_setCullMode },
	{ "getCullMode", w_Material3D_getCullMode },
	{ 0, 0 }
};

/**
 * Lights.
 **/

Light3D *luax_checklight3d(lua_State *L, int idx)
{
	return luax_checktype<Light3D>(L, idx);
}

static int w_Light3D_getType(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	lua_pushstring(L, getLightTypeString(t->getType()));
	return 1;
}

static int w_Light3D_setPosition(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	t->setPosition((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3), (float) luaL_checknumber(L, 4));
	return 0;
}

static int w_Light3D_getPosition(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	Vector3 pos = t->getPosition();
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	return 3;
}

static int w_Light3D_setDirection(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	luax_catchexcept(L, [&]()
	{
		t->setDirection((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3), (float) luaL_checknumber(L, 4));
	});
	return 0;
}

static int w_Light3D_getDirection(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	Vector3 dir = t->getDirection();
	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);
	return 3;
}

static int w_Light3D_setColor(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	t->setColor((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3), (float) luaL_checknumber(L, 4));
	return 0;
}

static int w_Light3D_getColor(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	Vector3 c = t->getColor();
	lua_pushnumber(L, c.x);
	lua_pushnumber(L, c.y);
	lua_pushnumber(L, c.z);
	return 3;
}

static int w_Light3D_setIntensity(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	luax_catchexcept(L, [&]() { t->setIntensity((float) luaL_checknumber(L, 2)); });
	return 0;
}

static int w_Light3D_getIntensity(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	lua_pushnumber(L, t->getIntensity());
	return 1;
}

static int w_Light3D_setRange(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	luax_catchexcept(L, [&]() { t->setRange((float) luaL_checknumber(L, 2)); });
	return 0;
}

static int w_Light3D_getRange(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	lua_pushnumber(L, t->getRange());
	return 1;
}

static int w_Light3D_setSpotAngles(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	luax_catchexcept(L, [&]() { t->setSpotAngles((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3)); });
	return 0;
}

static int w_Light3D_getSpotAngles(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	lua_pushnumber(L, t->getSpotInnerDegrees());
	lua_pushnumber(L, t->getSpotOuterDegrees());
	return 2;
}

static int w_Light3D_setFalloff(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	luax_catchexcept(L, [&]() { t->setFalloff((float) luaL_checknumber(L, 2)); });
	return 0;
}

static int w_Light3D_getFalloff(lua_State *L)
{
	Light3D *t = luax_checklight3d(L, 1);
	lua_pushnumber(L, t->getFalloff());
	return 1;
}

static const luaL_Reg w_Light3D_functions[] =
{
	{ "getType", w_Light3D_getType },
	{ "setPosition", w_Light3D_setPosition },
	{ "getPosition", w_Light3D_getPosition },
	{ "setDirection", w_Light3D_setDirection },
	{ "getDirection", w_Light3D_getDirection },
	{ "setColor", w_Light3D_setColor },
	{ "getColor", w_Light3D_getColor },
	{ "setIntensity", w_Light3D_setIntensity },
	{ "getIntensity", w_Light3D_getIntensity },
	{ "setRange", w_Light3D_setRange },
	{ "getRange", w_Light3D_getRange },
	{ "setSpotAngles", w_Light3D_setSpotAngles },
	{ "getSpotAngles", w_Light3D_getSpotAngles },
	{ "setFalloff", w_Light3D_setFalloff },
	{ "getFalloff", w_Light3D_getFalloff },
	{ 0, 0 }
};

/**
 * Meshes.
 **/

Mesh3D *luax_checkmesh3d(lua_State *L, int idx)
{
	return luax_checktype<Mesh3D>(L, idx);
}

static int w_Mesh3D_getVertexCount(lua_State *L)
{
	Mesh3D *t = luax_checkmesh3d(L, 1);
	lua_pushinteger(L, (lua_Integer) t->getVertexCount());
	return 1;
}

static int w_Mesh3D_getIndexCount(lua_State *L)
{
	Mesh3D *t = luax_checkmesh3d(L, 1);
	lua_pushinteger(L, (lua_Integer) t->getIndexCount());
	return 1;
}

static int w_Mesh3D_getBounds(lua_State *L)
{
	Vector3 minBounds, maxBounds;
	if (!luax_checkmesh3d(L, 1)->getBounds(minBounds, maxBounds))
	{
		lua_pushnil(L);
		return 1;
	}

	lua_pushnumber(L, minBounds.x);
	lua_pushnumber(L, minBounds.y);
	lua_pushnumber(L, minBounds.z);
	lua_pushnumber(L, maxBounds.x);
	lua_pushnumber(L, maxBounds.y);
	lua_pushnumber(L, maxBounds.z);
	return 6;
}

static int w_Mesh3D_setMaterial(lua_State *L)
{
	Mesh3D *t = luax_checkmesh3d(L, 1);
	Material3D *material = luax_checkmaterial3d(L, 2);
	luax_catchexcept(L, [&]() { t->setMaterial(material); });
	return 0;
}

static int w_Mesh3D_getMaterial(lua_State *L)
{
	Mesh3D *t = luax_checkmesh3d(L, 1);
	Material3D *material = t->getMaterial();

	if (material == nullptr)
		return 0;

	luax_pushtype(L, material);
	return 1;
}

static int w_Mesh3D_setLOD(lua_State *L)
{
	Mesh3D *mesh = luax_checkmesh3d(L, 1);
	float distance = (float) luaL_checknumber(L, 2);
	Mesh3D *lod = luax_checkmesh3d(L, 3);
	luax_catchexcept(L, [&]() { mesh->addLOD(distance, lod); });
	return 0;
}

static const luaL_Reg w_Mesh3D_functions[] =
{
	{ "getVertexCount", w_Mesh3D_getVertexCount },
	{ "getIndexCount", w_Mesh3D_getIndexCount },
	{ "getBounds", w_Mesh3D_getBounds },
	{ "setMaterial", w_Mesh3D_setMaterial },
	{ "getMaterial", w_Mesh3D_getMaterial },
	{ "setLOD", w_Mesh3D_setLOD },
	{ 0, 0 }
};

/**
 * Models.
 **/

static int w_Model3D_getPartCount(lua_State *L)
{
	Model3D *t = luax_checktype<Model3D>(L, 1);
	lua_pushinteger(L, (lua_Integer) t->getPartCount());
	return 1;
}

static int w_Model3D_getBounds(lua_State *L)
{
	Vector3 minBounds, maxBounds;
	if (!luax_checktype<Model3D>(L, 1)->getBounds(minBounds, maxBounds))
	{
		lua_pushnil(L);
		return 1;
	}

	lua_pushnumber(L, minBounds.x);
	lua_pushnumber(L, minBounds.y);
	lua_pushnumber(L, minBounds.z);
	lua_pushnumber(L, maxBounds.x);
	lua_pushnumber(L, maxBounds.y);
	lua_pushnumber(L, maxBounds.z);
	return 6;
}

static const luaL_Reg w_Model3D_functions[] =
{
	{ "getPartCount", w_Model3D_getPartCount },
	{ "getBounds", w_Model3D_getBounds },
	{ 0, 0 }
};

/**
 * Scene graph nodes.
 **/

static int w_Node3D_setParent(lua_State *L)
{
	Node3D *node = luax_checktype<Node3D>(L, 1);
	Node3D *parent = lua_isnoneornil(L, 2) ? nullptr : luax_checktype<Node3D>(L, 2);
	luax_catchexcept(L, [&]() { node->setParent(parent); });
	return 0;
}

static int w_Node3D_getParent(lua_State *L)
{
	Node3D *parent = luax_checktype<Node3D>(L, 1)->getParent();
	if (parent == nullptr)
		return 0;
	luax_pushtype(L, parent);
	return 1;
}

static int w_Node3D_addChild(lua_State *L)
{
	Node3D *node = luax_checktype<Node3D>(L, 1);
	Node3D *child = luax_checktype<Node3D>(L, 2);
	luax_catchexcept(L, [&]() { node->addChild(child); });
	return 0;
}

static int w_Node3D_removeChild(lua_State *L)
{
	Node3D *node = luax_checktype<Node3D>(L, 1);
	Node3D *child = luax_checktype<Node3D>(L, 2);
	node->removeChild(child);
	return 0;
}

static int w_Node3D_getChild(lua_State *L)
{
	Node3D *node = luax_checktype<Node3D>(L, 1);
	lua_Integer index = luaL_checkinteger(L, 2);
	if (index < 1)
		return luaL_argerror(L, 2, "index must be positive");
	Node3D *child = nullptr;
	luax_catchexcept(L, [&]() { child = node->getChild((size_t) (index - 1)); });
	luax_pushtype(L, child);
	return 1;
}

static int w_Node3D_setPosition(lua_State *L)
{
	Node3D *node = luax_checktype<Node3D>(L, 1);
	node->setPosition((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3), (float) luaL_checknumber(L, 4));
	return 0;
}

static int w_Node3D_setRotation(lua_State *L)
{
	Node3D *node = luax_checktype<Node3D>(L, 1);
	node->setRotation((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3), (float) luaL_checknumber(L, 4));
	return 0;
}

static int w_Node3D_setScale(lua_State *L)
{
	Node3D *node = luax_checktype<Node3D>(L, 1);
	node->setScale((float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3), (float) luaL_checknumber(L, 4));
	return 0;
}

static int w_Node3D_setMesh(lua_State *L)
{
	Node3D *node = luax_checktype<Node3D>(L, 1);
	if (lua_isnoneornil(L, 2))
		node->setMesh(nullptr);
	else if (luax_istype(L, 2, Mesh3D::type))
		node->setMesh(luax_checkmesh3d(L, 2));
	else if (luax_istype(L, 2, Model3D::type))
		node->setModel(luax_checktype<Model3D>(L, 2));
	else
		return luaL_argerror(L, 2, "expected a Mesh3D, Model3D, or nil");
	return 0;
}

static int w_Node3D_getWorldPosition(lua_State *L)
{
	Vector3 position = luax_checktype<Node3D>(L, 1)->getWorldPosition();
	lua_pushnumber(L, position.x);
	lua_pushnumber(L, position.y);
	lua_pushnumber(L, position.z);
	return 3;
}

static const luaL_Reg w_Node3D_functions[] =
{
	{ "setParent", w_Node3D_setParent },
	{ "getParent", w_Node3D_getParent },
	{ "addChild", w_Node3D_addChild },
	{ "removeChild", w_Node3D_removeChild },
	{ "getChild", w_Node3D_getChild },
	{ "setPosition", w_Node3D_setPosition },
	{ "setRotation", w_Node3D_setRotation },
	{ "setScale", w_Node3D_setScale },
	{ "setMesh", w_Node3D_setMesh },
	{ "getWorldPosition", w_Node3D_getWorldPosition },
	{ 0, 0 }
};

/**
 * Module functions.
 **/

static int w_newCamera(lua_State *L)
{
	Camera3D *camera = nullptr;
	luax_catchexcept(L, [&]() { camera = instance()->newCamera(); });
	luax_pushtype(L, camera);
	camera->release();
	return 1;
}

static int w_newMaterial(lua_State *L)
{
	Material3D *material = nullptr;
	luax_catchexcept(L, [&]() { material = instance()->newMaterial(); });
	luax_pushtype(L, material);
	material->release();
	return 1;
}

static int w_newNode(lua_State *L)
{
	Node3D *node = new Node3D();
	if (!lua_isnoneornil(L, 1))
	{
		if (luax_istype(L, 1, Mesh3D::type))
			node->setMesh(luax_checkmesh3d(L, 1));
		else if (luax_istype(L, 1, Model3D::type))
			node->setModel(luax_checktype<Model3D>(L, 1));
		else
		{
			node->release();
			return luaL_argerror(L, 1, "expected a Mesh3D, Model3D, or nil");
		}
	}
	luax_pushtype(L, node);
	node->release();
	return 1;
}

static int w_newLight(lua_State *L)
{
	const char *str = luaL_optstring(L, 1, "point");
	Light3D::LightType lightType;

	if (!lightTypeFromString(str, lightType))
		return luax_enumerror(L, "light type", str);

	Light3D *light = nullptr;
	luax_catchexcept(L, [&]() { light = instance()->newLight(lightType); });
	luax_pushtype(L, light);
	light->release();
	return 1;
}

static void fillVertex3DFromTable(lua_State *L, int idx, Vertex3D &vertex)
{
	// Precise format: { px, py, pz, nx, ny, nz, u, v, r, g, b, a }.
	// Omitted trailing fields keep the default values.
	vertex.position[0] = 0.0f;
	vertex.position[1] = 0.0f;
	vertex.position[2] = 0.0f;
	vertex.normal[0] = 0.0f;
	vertex.normal[1] = 1.0f;
	vertex.normal[2] = 0.0f;
	vertex.texcoord[0] = 0.0f;
	vertex.texcoord[1] = 0.0f;
	vertex.color[0] = 1.0f;
	vertex.color[1] = 1.0f;
	vertex.color[2] = 1.0f;
	vertex.color[3] = 1.0f;

	for (int i = 1; i <= 12; i++)
	{
		lua_rawgeti(L, idx, i);

		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			break;
		}

		float value = (float) lua_tonumber(L, -1);
		lua_pop(L, 1);

		switch (i)
		{
		case 1: vertex.position[0] = value; break;
		case 2: vertex.position[1] = value; break;
		case 3: vertex.position[2] = value; break;
		case 4: vertex.normal[0] = value; break;
		case 5: vertex.normal[1] = value; break;
		case 6: vertex.normal[2] = value; break;
		case 7: vertex.texcoord[0] = value; break;
		case 8: vertex.texcoord[1] = value; break;
		case 9: vertex.color[0] = value; break;
		case 10: vertex.color[1] = value; break;
		case 11: vertex.color[2] = value; break;
		case 12: vertex.color[3] = value; break;
		}
	}
}

/**
 * newMesh(vertices, indices, [usage])
 *
 * vertices: array of tables {px, py, pz, nx, ny, nz, u, v, r, g, b, a}
 * indices: array of 1-based indices into vertices, or nil to auto-fill
 * sequential triangles (3 vertices = 1 triangle).
 **/
static int w_newMesh(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	size_t vertexCount = luax_objlen(L, 1);

	if (vertexCount == 0)
		return luaL_error(L, "newMesh requires at least one vertex.");

	std::vector<Vertex3D> vertices;
	vertices.resize(vertexCount);

	for (size_t i = 0; i < vertexCount; i++)
	{
		lua_rawgeti(L, 1, (lua_Integer) (i + 1));
		luaL_checktype(L, -1, LUA_TTABLE);
		fillVertex3DFromTable(L, -1, vertices[i]);
		lua_pop(L, 1);
	}

	std::vector<uint32> indices;

	if (!lua_isnoneornil(L, 2))
	{
		luaL_checktype(L, 2, LUA_TTABLE);
		size_t indexCount = luax_objlen(L, 2);
		indices.reserve(indexCount);

		for (size_t i = 0; i < indexCount; i++)
		{
			lua_rawgeti(L, 2, (lua_Integer) (i + 1));
			lua_Integer value = luaL_checkinteger(L, -1);
			lua_pop(L, 1);

			if (value < 1 || (size_t) value > vertexCount)
				return luaL_error(L, "Mesh index %d is out of bounds (1..%d).", (int) (i + 1), (int) vertexCount);

			indices.push_back((uint32) (value - 1));
		}
	}
	else
	{
		// Sequential 1..N indexing.
		for (size_t i = 0; i < vertexCount; i++)
			indices.push_back((uint32) i);
	}

	love::graphics::BufferDataUsage usage = love::graphics::BUFFERDATAUSAGE_STATIC;

	if (!lua_isnoneornil(L, 3))
	{
		const char *str = luaL_checkstring(L, 3);

		if (!love::graphics::getConstant(str, usage))
			return luax_enumerror(L, "usage", love::graphics::getConstants(love::graphics::BUFFERDATAUSAGE_STATIC), str);
	}

	Mesh3D *mesh = nullptr;
	luax_catchexcept(L, [&]() { mesh = instance()->newMesh(vertices, indices, usage); });
	luax_pushtype(L, mesh);
	mesh->release();
	return 1;
}

/**
 * Primitive factory helpers: each pushes a Mesh3D created from the
 * corresponding primitives() generator.
 **/

static void pushPrimitiveMesh(lua_State *L, PrimitiveGeometry geo)
{
	Mesh3D *mesh = nullptr;
	luax_catchexcept(L, [&]() { mesh = instance()->newMesh(geo.vertices, geo.indices); });
	luax_pushtype(L, mesh);
	mesh->release();
}

static int w_newCube(lua_State *L)
{
	luax_catchexcept(L, [&]() { pushPrimitiveMesh(L, primitives::cube()); });
	return 1;
}

static int w_newBox(lua_State *L)
{
	float w = (float) luaL_optnumber(L, 1, 1.0);
	float h = (float) luaL_optnumber(L, 2, 1.0);
	float d = (float) luaL_optnumber(L, 3, 1.0);
	luax_catchexcept(L, [&]() { pushPrimitiveMesh(L, primitives::box(w, h, d)); });
	return 1;
}

static int w_newSphere(lua_State *L)
{
	int latitudes = (int) luaL_optinteger(L, 1, 16);
	int longitudes = (int) luaL_optinteger(L, 2, 24);
	float radius = (float) luaL_optnumber(L, 3, 1.0);
	luax_catchexcept(L, [&]() { pushPrimitiveMesh(L, primitives::sphere(latitudes, longitudes, radius)); });
	return 1;
}

static int w_newPlane(lua_State *L)
{
	int subdivisions = (int) luaL_optinteger(L, 1, 1);
	float size = (float) luaL_optnumber(L, 2, 1.0);
	luax_catchexcept(L, [&]() { pushPrimitiveMesh(L, primitives::plane(subdivisions, size)); });
	return 1;
}

static int w_newCylinder(lua_State *L)
{
	int slices = (int) luaL_optinteger(L, 1, 24);
	float radius = (float) luaL_optnumber(L, 2, 1.0);
	float height = (float) luaL_optnumber(L, 3, 1.0);
	luax_catchexcept(L, [&]() { pushPrimitiveMesh(L, primitives::cylinder(slices, radius, height)); });
	return 1;
}

static int w_newCone(lua_State *L)
{
	int slices = (int) luaL_optinteger(L, 1, 24);
	float radius = (float) luaL_optnumber(L, 2, 1.0);
	float height = (float) luaL_optnumber(L, 3, 1.0);
	luax_catchexcept(L, [&]() { pushPrimitiveMesh(L, primitives::cone(slices, radius, height)); });
	return 1;
}

static int w_newCapsule(lua_State *L)
{
	int slices = (int) luaL_optinteger(L, 1, 24);
	int rings = (int) luaL_optinteger(L, 2, 8);
	float radius = (float) luaL_optnumber(L, 3, 0.5);
	float height = (float) luaL_optnumber(L, 4, 1.0);
	luax_catchexcept(L, [&]() { pushPrimitiveMesh(L, primitives::capsule(slices, rings, radius, height)); });
	return 1;
}

static int w_newTorus(lua_State *L)
{
	int segments = (int) luaL_optinteger(L, 1, 32);
	int rings = (int) luaL_optinteger(L, 2, 16);
	float major = (float) luaL_optnumber(L, 3, 1.0);
	float minor = (float) luaL_optnumber(L, 4, 0.3);
	luax_catchexcept(L, [&]() { pushPrimitiveMesh(L, primitives::torus(segments, rings, major, minor)); });
	return 1;
}

/**
 * Material-lookup context for newModel. Resolves MTL texture filenames to
 * love.graphics Textures through an optional Lua table passed to newModel,
 * filling in the MTL record's other values.
 **/
struct ModelLoaderContext
{
	lua_State *L;
	int textureTableIndex;

	love::graphics::Texture *findTexture(const std::string &filename)
	{
		if (filename.empty() || textureTableIndex == 0)
			return nullptr;

		lua_pushvalue(L, textureTableIndex);
		lua_pushstring(L, filename.c_str());
		lua_gettable(L, -2);

		love::graphics::Texture *texture = nullptr;

		if (!lua_isnil(L, -1))
			texture = luax_totype<love::graphics::Texture>(L, -1, love::graphics::Texture::type);

		lua_pop(L, 2);
		return texture;
	}
};

static void setupMaterialFromRecord(Material3D *material, const OBJLoader::MaterialRecord &record, ModelLoaderContext *ctx)
{
	material->setColor(record.diffuse.x, record.diffuse.y, record.diffuse.z, record.opacity);
	material->setSpecularParams((record.specular.x + record.specular.y + record.specular.z) / 3.0f, record.shininess);
	material->setEmissiveColor(record.emissive);

	if (record.opacity < 1.0f)
		material->setOpacity(record.opacity);

	if (ctx != nullptr)
	{
		if (love::graphics::Texture *tex = ctx->findTexture(record.diffuseTexture))
			material->setDiffuseTexture(tex);

		if (love::graphics::Texture *tex = ctx->findTexture(record.specularTexture))
			material->setSpecularTexture(tex);

		if (love::graphics::Texture *tex = ctx->findTexture(record.emissiveTexture))
			material->setEmissiveTexture(tex);

		if (love::graphics::Texture *tex = ctx->findTexture(record.normalTexture))
			material->setNormalTexture(tex);
	}
}

/**
 * newModel(objContent, [mtlContent], [textures])
 * → Model3D (returns model, materials table).
 **/
static int w_newModel(lua_State *L)
{
	size_t objSize = 0;
	const char *objData = luaL_checklstring(L, 1, &objSize);

	std::string mtlContent;
	if (!lua_isnoneornil(L, 2) && luaL_checkstring(L, 2))
		mtlContent = lua_tostring(L, 2);

	// Optional texture table keyed by the texture filenames in the MTL.
	int textureTableIndex = 0;
	if (lua_istable(L, 3))
		textureTableIndex = 3;

	OBJLoader loader;

	if (!loader.parse(objData, objSize))
		return luaL_error(L, "Could not load OBJ model: %s", loader.getError().c_str());

	if (!mtlContent.empty())
		loader.parseMTL(mtlContent.data(), mtlContent.size());

	ModelLoaderContext ctx = { L, textureTableIndex };

	Model3D *model = nullptr;
	std::vector<StrongRef<Material3D>> materials;

	try
	{
		model = instance()->newModel(loader, love::graphics::BUFFERDATAUSAGE_STATIC,
			[&](const OBJLoader::MaterialRecord &record) -> Material3D *
			{
				Material3D *material = new Material3D();
				setupMaterialFromRecord(material, record, &ctx);

				// The vector takes over the creation reference; the pointer
				// handed to the loader is borrowed (addPart retains it).
				materials.emplace_back(material);
				material->release();

				return material;
			});
	}
	catch (love::Exception &e)
	{
		return luaL_error(L, "Could not build OBJ model: %s", e.what());
	}

	luax_pushtype(L, model);
	model->release();

	// Return the (unique) materials created for direct tweaking.
	lua_newtable(L);

	size_t outIndex = 1;
	std::vector<Material3D *> emitted;

	for (const auto &m : materials)
	{
		// The same material object can back multiple groups.
		bool emittedAlready = false;
		for (Material3D *e : emitted)
		{
			if (e == m.get())
			{
				emittedAlready = true;
				break;
			}
		}

		if (emittedAlready)
			continue;

		emitted.push_back(m.get());
		lua_pushinteger(L, (lua_Integer) outIndex++);
		luax_pushtype(L, m.get());
		lua_settable(L, -3);
	}

	return 2;
}

static int w_newModelFromFile(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	auto *filesystem = Module::getInstance<love::filesystem::Filesystem>(Module::M_FILESYSTEM);
	auto *imageModule = Module::getInstance<love::image::Image>(Module::M_IMAGE);
	auto *graphics = Module::getInstance<love::graphics::Graphics>(Module::M_GRAPHICS);
	if (filesystem == nullptr || imageModule == nullptr || graphics == nullptr)
		return luaL_error(L, "newModelFromFile requires love.filesystem, love.image, and love.graphics.");

	auto readFile = [filesystem](const std::string &filename) -> std::vector<uint8>
	{
		StrongRef<love::filesystem::FileData> file(filesystem->read(filename.c_str()), Acquire::NORETAIN);
		if (file.get() == nullptr)
			throw love::Exception("Could not read model asset '%s'.", filename.c_str());
		return std::vector<uint8>((const uint8 *) file->getData(),
			(const uint8 *) file->getData() + file->getSize());
	};

	std::map<std::string, StrongRef<love::graphics::Texture>> textureCache;
	auto makeTexture = [imageModule, graphics](const std::vector<uint8> &bytes,
		const love::graphics::SamplerState &sampler, bool linear) -> love::graphics::Texture *
	{
		love::filesystem::FileData file(bytes.size(), "model-texture");
		if (!bytes.empty())
			memcpy(file.getData(), bytes.data(), bytes.size());
		StrongRef<love::image::ImageData> imageData;
		imageData.set(imageModule->newImageData(&file), Acquire::NORETAIN);

		love::graphics::Texture::Settings settings;
		settings.linear = linear;
		settings.mipmaps = sampler.mipmapFilter == love::graphics::SamplerState::MIPMAP_FILTER_NONE
			? love::graphics::Texture::MIPMAPS_NONE : love::graphics::Texture::MIPMAPS_AUTO;
		love::graphics::Texture::Slices slices(love::graphics::TEXTURE_2D);
		slices.set(0, 0, imageData.get());
		love::graphics::Texture *texture = graphics->newTexture(settings, &slices);
		try
		{
			texture->setSamplerState(sampler);
		}
		catch (...)
		{
			texture->release();
			throw;
		}
		return texture;
	};

	try
	{
		std::string filename(path);
		size_t dot = filename.find_last_of('.');
		std::string extension = dot == std::string::npos ? "" : filename.substr(dot + 1);
		std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char) tolower(c); });
		Model3D *model = nullptr;

		if (extension == "gltf" || extension == "glb")
		{
			auto cachedLoadTexture = [&](const std::vector<uint8> &bytes,
				const love::graphics::SamplerState &sampler, bool linear) -> love::graphics::Texture *
			{
				return makeTexture(bytes, sampler, linear);
			};
			model = GLTFLoader::load(filename, graphics, readFile, cachedLoadTexture);
		}
		else if (extension == "obj")
		{
			std::vector<uint8> obj = readFile(filename);
			OBJLoader loader;
			if (!loader.parse((const char *) obj.data(), obj.size()))
				throw love::Exception("Could not load OBJ model: %s", loader.getError().c_str());

			auto resolveRelativePath = [](const std::string &baseFile, const std::string &relative)
			{
				if (relative.empty() || relative[0] == '/' || relative.find(':') != std::string::npos)
					throw love::Exception("OBJ resource path '%s' must be relative.", relative.c_str());
				size_t slash = baseFile.find_last_of('/');
				std::string combined = (slash == std::string::npos ? std::string() : baseFile.substr(0, slash + 1))
					+ relative;
				std::vector<std::string> components;
				size_t start = 0;
				while (start <= combined.size())
				{
					size_t end = combined.find('/', start);
					std::string component = combined.substr(start,
						end == std::string::npos ? std::string::npos : end - start);
					if (!component.empty() && component != ".")
					{
						if (component == "..")
						{
							if (components.empty())
								throw love::Exception("OBJ resource path '%s' escapes the filesystem root.", relative.c_str());
							components.pop_back();
						}
						else
							components.push_back(component);
					}
					if (end == std::string::npos) break;
					start = end + 1;
				}
				std::string resolved;
				for (const std::string &component : components)
				{
					if (!resolved.empty()) resolved += '/';
					resolved += component;
				}
				return resolved;
			};
			for (const std::string &mtlName : loader.getMaterialLibraries())
			{
				std::string mtlPath = resolveRelativePath(filename, mtlName);
				std::vector<uint8> mtl = readFile(mtlPath);
				size_t firstMaterial = loader.getMaterials().size();
				loader.parseMTL((const char *) mtl.data(), mtl.size());
				for (size_t i = firstMaterial; i < loader.getMaterials().size(); i++)
				{
					OBJLoader::MaterialRecord &record = loader.getMaterials()[i];
					auto resolve = [&](std::string &texture)
					{
						if (!texture.empty())
							texture = resolveRelativePath(mtlPath, texture);
					};
					resolve(record.diffuseTexture);
					resolve(record.specularTexture);
					resolve(record.emissiveTexture);
					resolve(record.normalTexture);
				}
			}

			std::map<std::string, StrongRef<love::graphics::Texture>> objTextures;
			std::vector<StrongRef<Material3D>> createdMaterials;
			model = instance()->newModel(loader, love::graphics::BUFFERDATAUSAGE_STATIC,
				[&](const OBJLoader::MaterialRecord &record) -> Material3D *
				{
					Material3D *material = new Material3D();
					createdMaterials.emplace_back(material);
					material->release();
					setupMaterialFromRecord(material, record, nullptr);
					auto bind = [&](const std::string &texturePath, int slot)
					{
						if (texturePath.empty())
							return;
						auto it = objTextures.find(texturePath);
						if (it == objTextures.end())
						{
							std::vector<uint8> bytes = readFile(texturePath);
							love::graphics::SamplerState sampler;
							love::graphics::Texture *texture = makeTexture(bytes, sampler, false);
							try
							{
								it = objTextures.emplace(texturePath, StrongRef<love::graphics::Texture>(texture)).first;
							}
							catch (...)
							{
								texture->release();
								throw;
							}
							texture->release();
						}
						if (slot == 0) material->setDiffuseTexture(it->second.get());
						else if (slot == 1) material->setSpecularTexture(it->second.get());
						else if (slot == 2) material->setEmissiveTexture(it->second.get());
						else material->setNormalTexture(it->second.get());
					};
					bind(record.diffuseTexture, 0);
					bind(record.specularTexture, 1);
					bind(record.emissiveTexture, 2);
					bind(record.normalTexture, 3);
					return material;
				});
		}
		else
			throw love::Exception("newModelFromFile supports .obj, .gltf, and .glb files (got '%s').", extension.c_str());

		luax_pushtype(L, model);
		model->release();
		return 1;
	}
	catch (love::Exception &e)
	{
		return luaL_error(L, "Could not load model file '%s': %s", path, e.what());
	}
}

static int w_setCamera(lua_State *L)
{
	Camera3D *camera = luax_checkcamera3d(L, 1);
	luax_catchexcept(L, [&]() { instance()->setCamera(camera); });
	return 0;
}

static int w_getCamera(lua_State *L)
{
	Camera3D *camera = instance()->getCamera();
	if (camera == nullptr)
		return 0;

	luax_pushtype(L, camera);
	return 1;
}

static int w_addLight(lua_State *L)
{
	Light3D *light = luax_checklight3d(L, 1);
	luax_catchexcept(L, [&]() { instance()->addLight(light); });
	return 0;
}

static int w_removeLight(lua_State *L)
{
	Light3D *light = luax_checklight3d(L, 1);
	luax_catchexcept(L, [&]() { instance()->removeLight(light); });
	return 0;
}

static int w_clearLights(lua_State *L)
{
	instance()->clearLights();
	return 0;
}

static int w_setAmbientColor(lua_State *L)
{
	instance()->setAmbientColor((float) luaL_checknumber(L, 1), (float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3));
	return 0;
}

static int w_getAmbientColor(lua_State *L)
{
	Vector3 ambient = instance()->getAmbientColor();
	lua_pushnumber(L, ambient.x);
	lua_pushnumber(L, ambient.y);
	lua_pushnumber(L, ambient.z);
	return 3;
}

/**
 * Matrix building for draw calls: position + optional euler angles +
 * optional scale.
 *
 * draw(mesh_or_model, x, y, z, [rotX, rotY, rotZ], [sx, sy, sz])
 **/
static Matrix4 buildDrawMatrix(lua_State *L, int startidx)
{
	float x = (float) luaL_checknumber(L, startidx);
	float y = (float) luaL_checknumber(L, startidx + 1);
	float z = (float) luaL_checknumber(L, startidx + 2);

	Vector3 rotationDegrees(0.0f, 0.0f, 0.0f);

	if (lua_gettop(L) >= startidx + 3)
	{
		rotationDegrees = Vector3(
			(float) luaL_checknumber(L, startidx + 3),
			(float) luaL_checknumber(L, startidx + 4),
			(float) luaL_checknumber(L, startidx + 5));
	}

	Vector3 scale(1.0f, 1.0f, 1.0f);

	if (lua_gettop(L) >= startidx + 6)
	{
		scale = Vector3(
			(float) luaL_checknumber(L, startidx + 6),
			(float) luaL_checknumber(L, startidx + 7),
			(float) luaL_checknumber(L, startidx + 8));
	}

	Matrix4 matrix;
	luax_catchexcept(L, [&]()
	{
		Quaternion rotation = Quaternion::fromEuler(
			rotationDegrees.x * 3.14159265359f / 180.0f,
			rotationDegrees.y * 3.14159265359f / 180.0f,
			rotationDegrees.z * 3.14159265359f / 180.0f);
		matrix.setTransformQ(Vector3(x, y, z), rotation, scale);
	});

	return matrix;
}

static int w_draw(lua_State *L)
{
	Graphics3D *g = instance();
	if (luax_istype(L, 1, Node3D::type))
	{
		luax_catchexcept(L, [&]() { g->drawNode(luax_checktype<Node3D>(L, 1)); });
		return 0;
	}

	Matrix4 matrix = buildDrawMatrix(L, 2);

	luax_catchexcept(L, [&]()
	{
		if (luax_istype(L, 1, Model3D::type))
		{
			Model3D *model = luax_checktype<Model3D>(L, 1);
			g->drawModel(model, matrix);
		}
		else
		{
			Mesh3D *mesh = luax_checkmesh3d(L, 1);
			g->drawMesh(mesh, matrix);
		}
	});

	return 0;
}

static int w_drawInstanced(lua_State *L)
{
	Mesh3D *mesh = luax_checkmesh3d(L, 1);
	Graphics3D *graphics3d = instance();
	std::vector<Matrix4> transforms;

	if (luax_istype(L, 2, love::Data::type))
	{
		love::Data *data = luax_checktype<love::Data>(L, 2);
		if (data->getSize() % (sizeof(float) * 16) != 0)
			return luaL_argerror(L, 2, "ByteData size must be a multiple of 64 bytes (column-major float32 mat4 transforms)");
		size_t count = data->getSize() / (sizeof(float) * 16);
		const float *values = (const float *) data->getData();
		transforms.resize(count);
		for (size_t i = 0; i < count; i++)
			memcpy(transforms[i].m, values + i * 16, sizeof(float) * 16);
	}
	else
	{
		luaL_checktype(L, 2, LUA_TTABLE);
		size_t count = luax_objlen(L, 2);
		transforms.reserve(count);
		for (size_t i = 1; i <= count; i++)
		{
			lua_rawgeti(L, 2, (lua_Integer) i);
			luaL_checktype(L, -1, LUA_TTABLE);
			size_t valueCount = luax_objlen(L, -1);
			if (valueCount != 9 && valueCount != 16)
			{
				lua_pop(L, 1);
				return luaL_error(L, "Each instance transform must contain 9 TRS values or 16 column-major matrix values.");
			}

			float values[16] = {};
			for (size_t value = 0; value < valueCount; value++)
			{
				lua_rawgeti(L, -1, (lua_Integer) (value + 1));
				values[value] = (float) luaL_checknumber(L, -1);
				lua_pop(L, 1);
			}

			Matrix4 matrix;
			if (valueCount == 16)
				memcpy(matrix.m, values, sizeof(values));
			else
			{
				Quaternion rotation = Quaternion::fromEuler(values[3] * 3.14159265359f / 180.0f,
					values[4] * 3.14159265359f / 180.0f, values[5] * 3.14159265359f / 180.0f);
				matrix.setTransformQ(Vector3(values[0], values[1], values[2]), rotation,
					Vector3(values[6], values[7], values[8]));
			}
			transforms.push_back(matrix);
			lua_pop(L, 1);
		}
	}

	luax_catchexcept(L, [&]()
	{
		graphics3d->drawInstanced(mesh, transforms);
	});
	return 0;
}

static Vector4 getDebugColor(lua_State *L, int index, const Vector4 &defaultColor)
{
	return Vector4((float) luaL_optnumber(L, index, defaultColor.x),
		(float) luaL_optnumber(L, index + 1, defaultColor.y),
		(float) luaL_optnumber(L, index + 2, defaultColor.z),
		(float) luaL_optnumber(L, index + 3, defaultColor.w));
}

static int w_debugLine(lua_State *L)
{
	Vector3 from((float) luaL_checknumber(L, 1), (float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3));
	Vector3 to((float) luaL_checknumber(L, 4), (float) luaL_checknumber(L, 5), (float) luaL_checknumber(L, 6));
	Vector4 color(1.0f, 0.85f, 0.1f, 1.0f);
	bool depthTest = true;
	if (lua_isboolean(L, 7))
		depthTest = lua_toboolean(L, 7) != 0;
	else
	{
		color = getDebugColor(L, 7, color);
		if (!lua_isnoneornil(L, 11))
			depthTest = lua_toboolean(L, 11) != 0;
	}
	luax_catchexcept(L, [&]() { instance()->debugLine(from, to, color, depthTest); });
	return 0;
}

static int w_debugBox(lua_State *L)
{
	Graphics3D *g = instance();
	Vector4 color = Vector4(0.2f, 0.9f, 1.0f, 1.0f);
	bool depthTest = true;

	luax_catchexcept(L, [&]()
	{
		if (lua_isnumber(L, 1))
		{
			Vector3 boundsMin(
				(float) luaL_checknumber(L, 1),
				(float) luaL_checknumber(L, 2),
				(float) luaL_checknumber(L, 3));
			Vector3 boundsMax(
				(float) luaL_checknumber(L, 4),
				(float) luaL_checknumber(L, 5),
				(float) luaL_checknumber(L, 6));
			if (lua_isboolean(L, 7))
				depthTest = lua_toboolean(L, 7) != 0;
			else
			{
				color = getDebugColor(L, 7, color);
				if (!lua_isnoneornil(L, 11))
					depthTest = lua_toboolean(L, 11) != 0;
			}
			g->debugBox(boundsMin, boundsMax, color, depthTest);
			return;
		}

		Vector3 boundsMin, boundsMax;
		if (luax_istype(L, 1, Model3D::type))
		{
			if (!luax_checktype<Model3D>(L, 1)->getBounds(boundsMin, boundsMax))
				throw love::Exception("Cannot draw bounds for an empty Model3D.");
		}
		else if (!luax_checkmesh3d(L, 1)->getBounds(boundsMin, boundsMax))
			throw love::Exception("Cannot draw bounds for an empty Mesh3D.");

		if (!g->isDrawing())
			throw love::Exception("debugBox can only be called between begin3D and end3d.");

		Matrix4 modelMatrix = buildDrawMatrix(L, 2);
		for (float value : modelMatrix.m)
		{
			if (!std::isfinite(value))
				throw love::Exception("Debug box transform values must be finite.");
		}
		if (!lua_isnoneornil(L, 11))
			depthTest = lua_toboolean(L, 11) != 0;

		Vector3 corners[8];
		for (int i = 0; i < 8; i++)
		{
			Vector3 local((i & 1) ? boundsMax.x : boundsMin.x,
				(i & 2) ? boundsMax.y : boundsMin.y,
				(i & 4) ? boundsMax.z : boundsMin.z);
			corners[i] = modelMatrix * local;
		}
		static const int edges[12][2] = {
			{ 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 },
			{ 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
			{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
		};
		for (const auto &edge : edges)
			g->debugLine(corners[edge[0]], corners[edge[1]], color, depthTest);
	});

	return 0;
}

static int w_debugSphere(lua_State *L)
{
	Vector3 center((float) luaL_checknumber(L, 1), (float) luaL_checknumber(L, 2), (float) luaL_checknumber(L, 3));
	float radius = (float) luaL_checknumber(L, 4);
	Vector4 color(1.0f, 0.3f, 0.15f, 1.0f);
	bool depthTest = true;
	if (lua_isboolean(L, 5))
		depthTest = lua_toboolean(L, 5) != 0;
	else
	{
		color = getDebugColor(L, 5, color);
		if (!lua_isnoneornil(L, 9))
			depthTest = lua_toboolean(L, 9) != 0;
	}
	luax_catchexcept(L, [&]() { instance()->debugSphere(center, radius, color, depthTest); });
	return 0;
}

static int w_debugNormals(lua_State *L)
{
	Mesh3D *mesh = luax_checkmesh3d(L, 1);
	Matrix4 modelMatrix = buildDrawMatrix(L, 2);
	float length = 0.25f;
	bool depthTest = true;
	if (lua_isboolean(L, 11))
		depthTest = lua_toboolean(L, 11) != 0;
	else
	{
		length = (float) luaL_optnumber(L, 11, 0.25);
		if (!lua_isnoneornil(L, 12))
			depthTest = lua_toboolean(L, 12) != 0;
	}
	luax_catchexcept(L, [&]() { instance()->debugNormals(mesh, modelMatrix, length, depthTest); });
	return 0;
}

static int wbegin3D(lua_State *L)
{
	luax_catchexcept(L, [&]() { instance()->begin3D(); });
	return 0;
}

static int wend3D(lua_State *L)
{
	luax_catchexcept(L, [&]() { instance()->end3D(); });
	return 0;
}

static int w_clearDepth(lua_State *L)
{
	luax_catchexcept(L, [&]() { instance()->clearDepth(); });
	return 0;
}

static int w_setWinding(lua_State *L)
{
	const char *str = luaL_checkstring(L, 1);
	love::graphics::Winding winding;

	if (!love::graphics::getConstant(str, winding))
		return luax_enumerror(L, "winding", love::graphics::getConstants(love::graphics::WINDING_CCW), str);

	instance()->setWinding(winding);
	return 0;
}

static int w_getWinding(lua_State *L)
{
	const char *windingstr;
	if (!love::graphics::getConstant(instance()->getWinding(), windingstr))
		return luaL_error(L, "Unknown winding.");

	lua_pushstring(L, windingstr);
	return 1;
}

static int w_setAutoBatching(lua_State *L)
{
	instance()->setAutoBatching(luax_checkboolean(L, 1));
	return 0;
}

static int w_isAutoBatchingEnabled(lua_State *L)
{
	lua_pushboolean(L, instance()->isAutoBatchingEnabled());
	return 1;
}

static int w_getStats(lua_State *L)
{
	Graphics3D::Stats3D stats = instance()->getStats();

	lua_createtable(L, 0, 8);
	lua_pushinteger(L, stats.drawsPassed);
	lua_setfield(L, -2, "drawsPassed");
	lua_pushinteger(L, stats.drawsFlushed);
	lua_setfield(L, -2, "drawsFlushed");
	lua_pushinteger(L, stats.drawCalls);
	lua_setfield(L, -2, "drawCalls");
	lua_pushinteger(L, stats.triangles);
	lua_setfield(L, -2, "triangles");
	lua_pushinteger(L, stats.culled);
	lua_setfield(L, -2, "culled");
	lua_pushinteger(L, stats.lightsUsed);
	lua_setfield(L, -2, "lightsUsed");
	lua_pushinteger(L, stats.queued);
	lua_setfield(L, -2, "queued");
	lua_pushinteger(L, LOVE3D_MAX_LIGHTS);
	lua_setfield(L, -2, "maxLights");

	return 1;
}

// List of functions to expose on the module table.
static const luaL_Reg functions[] =
{
	{ "newCamera", w_newCamera },
	{ "newMaterial", w_newMaterial },
	{ "newNode", w_newNode },
	{ "newLight", w_newLight },
	{ "newMesh", w_newMesh },
	{ "newCube", w_newCube },
	{ "newBox", w_newBox },
	{ "newSphere", w_newSphere },
	{ "newPlane", w_newPlane },
	{ "newCylinder", w_newCylinder },
	{ "newCone", w_newCone },
	{ "newCapsule", w_newCapsule },
	{ "newTorus", w_newTorus },
	{ "newModel", w_newModel },
	{ "newModelFromFile", w_newModelFromFile },
	{ "setCamera", w_setCamera },
	{ "getCamera", w_getCamera },
	{ "addLight", w_addLight },
	{ "removeLight", w_removeLight },
	{ "clearLights", w_clearLights },
	{ "setAmbientColor", w_setAmbientColor },
	{ "getAmbientColor", w_getAmbientColor },
	{ "begin3d", wbegin3D },
	{ "end3d", wend3D },
	{ "draw", w_draw },
	{ "drawInstanced", w_drawInstanced },
	{ "debugLine", w_debugLine },
	{ "debugBox", w_debugBox },
	{ "debugSphere", w_debugSphere },
	{ "debugNormals", w_debugNormals },
	{ "clearDepth", w_clearDepth },
	{ "setWinding", w_setWinding },
	{ "getWinding", w_getWinding },
	{ "setAutoBatching", w_setAutoBatching },
	{ "isAutoBatchingEnabled", w_isAutoBatchingEnabled },
	{ "getStats", w_getStats },
	{ 0, 0 }
};

extern "C" int luaopen_love_graphics3d(lua_State *L)
{
	Graphics3D *instance = instance();
	if (instance == nullptr)
	{
		luax_catchexcept(L, [&]() { instance = new love::graphics3d::Graphics3D(); });
	}
	else
		instance->retain();

	WrappedModule w;
	w.module = instance;
	w.name = "graphics3d";
	w.type = &Graphics3D::type;
	w.functions = functions;

	// Types: luaopen_X-style openers register each type's methods.
	static const lua_CFunction types[] =
	{
		camera3d_opentype,
		material3d_opentype,
		light3d_opentype,
		mesh3d_opentype,
		model3d_opentype,
		node3d_opentype,
		nullptr
	};

	w.types = types;

	return luax_register_module(L, w);
}

static int camera3d_opentype(lua_State *L)
{
	return luax_register_type(L, &Camera3D::type, w_Camera3D_functions, nullptr);
}

static int material3d_opentype(lua_State *L)
{
	return luax_register_type(L, &Material3D::type, w_Material3D_functions, nullptr);
}

static int light3d_opentype(lua_State *L)
{
	return luax_register_type(L, &Light3D::type, w_Light3D_functions, nullptr);
}

static int mesh3d_opentype(lua_State *L)
{
	return luax_register_type(L, &Mesh3D::type, w_Mesh3D_functions, nullptr);
}

static int model3d_opentype(lua_State *L)
{
	return luax_register_type(L, &Model3D::type, w_Model3D_functions, nullptr);
}

static int node3d_opentype(lua_State *L)
{
	return luax_register_type(L, &Node3D::type, w_Node3D_functions, nullptr);
}
} // graphics3d
} // love
