--
-- LOVING3D - Rubik's Cube interactive 3D benchmark.
--
-- Copyright (c) 2026 Saugat Pokharel
--
-- This file is part of LOVING3D, an independent extension adding native 3D
-- capabilities to the LOVE game engine (love2d.org). The underlying engine
-- remains freely distributable under the terms of its original zlib license
-- (see license.txt).
--
-- Exercises the love.graphics3d module end to end:
--   * newMesh with custom vertices, per-vertex colors and indices
--   * every primitive generator (box, sphere, capsule, cone, cylinder,
--     torus, plane) with per-object transforms and scale
--   * all three light types (directional, point, spot) + ambient color
--   * Blinn-Phong materials: specular params, emissive, unlit, textures
--   * mouse-drag layer turns via raycast picking (real cube-sim feel)
--   * camera orbit with inertia, zoom, perspective/ortho toggle
--   * 2D/3D state coexistence (sky gradient + HUD around the 3D pass)
--   * getStats()
--
-- Controls:
--   Left-drag on cube   turn the layer under the cursor
--   Left-drag off cube  orbit the camera (with inertia)
--   Right-drag          orbit the camera
--   Mouse wheel         zoom
--   U D L R F B         turn faces (hold shift for reverse)
--   Space               one random turn
--   S                   scramble (25 random turns)
--   R                   reset the cube
--   Enter               toggle the auto-demo
--   O                   toggle perspective/orthographic
--   G                   toggle the showcase ring
--   C                   reset the camera
--   H                   toggle this help panel
--   Esc                 quit
--

local g3d = love.graphics3d

--------------------------------------------------------------------------
-- Math (vectors and quaternions as tables, matching the module's C++
-- conventions: right-handed, +Y up, draw() euler = qY * qX * qZ)
--------------------------------------------------------------------------

local atan2 = math.atan2 or math.atan

local function v3(x, y, z) return { x = x, y = y, z = z } end

local function vSub(a, b) return v3(a.x - b.x, a.y - b.y, a.z - b.z) end
local function vAdd(a, b) return v3(a.x + b.x, a.y + b.y, a.z + b.z) end
local function vScale(a, s) return v3(a.x * s, a.y * s, a.z * s) end
local function vDot(a, b) return a.x * b.x + a.y * b.y + a.z * b.z end
local function vCross(a, b)
	return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x)
end
local function vLen(a) return math.sqrt(vDot(a, a)) end
local function vNorm(a)
	local l = vLen(a)
	if l <= 1e-9 then return v3(0, 0, 0) end
	return vScale(a, 1.0 / l)
end

local function quatIdentity() return { 0, 0, 0, 1 } end

local function quatMul(a, b)
	return {
		a[4] * b[1] + a[1] * b[4] + a[2] * b[3] - a[3] * b[2],
		a[4] * b[2] - a[1] * b[3] + a[2] * b[4] + a[3] * b[1],
		a[4] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[4],
		a[4] * b[4] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3],
	}
end

local function quatFromAxisAngle(ax, ay, az, radians)
	local s = math.sin(radians * 0.5)
	return { ax * s, ay * s, az * s, math.cos(radians * 0.5) }
end

local function quatNormalize(q)
	local len = math.sqrt(q[1] * q[1] + q[2] * q[2] + q[3] * q[3] + q[4] * q[4])
	if len > 1e-9 then
		return { q[1] / len, q[2] / len, q[3] / len, q[4] / len }
	end
	return q
end

local function quatRotate(q, x, y, z)
	-- v' = v + 2*w*(u x v) + 2*(u x (u x v)) with u = (qx, qy, qz).
	local ux, uy, uz = q[1], q[2], q[3]
	local cx = uy * z - uz * y
	local cy = uz * x - ux * z
	local cz = ux * y - uy * x
	local dx = uy * cz - uz * cy
	local dy = uz * cx - ux * cz
	local dz = ux * cy - uy * cx
	return x + 2.0 * q[4] * cx + 2.0 * dx,
		y + 2.0 * q[4] * cy + 2.0 * dy,
		z + 2.0 * q[4] * cz + 2.0 * dz
end

-- Inverse of the engine's Quaternion::fromEuler (q = qY * qX * qZ).
local function quatToEulerDegrees(q)
	local x, y, z, w = q[1], q[2], q[3], q[4]
	local test = x * y + z * w
	local rx, ry, rz

	if test > 0.499 then
		rz = 2.0 * atan2(x, w)
		ry = 0.0
		rx = math.pi * 0.5
	elseif test < -0.499 then
		rz = -2.0 * atan2(x, w)
		ry = 0.0
		rx = -math.pi * 0.5
	else
		rx = atan2(2.0 * y * w - 2.0 * x * z, 1.0 - 2.0 * y * y - 2.0 * z * z)
		rz = math.asin(math.max(-1.0, math.min(1.0, 2.0 * test)))
		ry = atan2(2.0 * x * w - 2.0 * y * z, 1.0 - 2.0 * x * x - 2.0 * z * z)
	end

	return math.deg(rx), math.deg(ry), math.deg(rz)
end

-- Axis-aligned 90-degree turns keep quaternion components in a small set;
-- snapping to it keeps orientations exact forever.
local SNAP_VALUES = { 0.0, 0.5, math.sqrt(0.5), 1.0 }

local function quatSnapAxisAligned(q)
	local out = {}
	for i = 1, 4 do
		local best, bestdist = 0.0, math.huge
		for _, v in ipairs(SNAP_VALUES) do
			local d = math.abs(math.abs(q[i]) - v)
			if d < bestdist then
				bestdist = d
				best = v
			end
		end
		out[i] = (q[i] < 0.0) and -best or best
	end
	return quatNormalize(out)
end

local function easeOutCubic(t)
	t = math.min(1.0, math.max(0.0, t))
	local u = 1.0 - t
	return 1.0 - u * u * u
end

--------------------------------------------------------------------------
-- Cube construction
--------------------------------------------------------------------------

local CELL = 1.0    -- world distance between cubie centers
local CUBIE = 0.94  -- cubie size as a fraction of the cell
local STICKER = 0.80     -- sticker size as a fraction of the cubie face
local STICKER_LIFT = 0.006

local FACE_COLORS = {
	px = { 0.90, 0.15, 0.15 }, -- +X red
	nx = { 0.95, 0.50, 0.10 }, -- -X orange
	py = { 0.95, 0.95, 0.95 }, -- +Y white
	ny = { 0.95, 0.90, 0.15 }, -- -Y yellow
	pz = { 0.15, 0.75, 0.25 }, -- +Z green
	nz = { 0.20, 0.40, 0.95 }, -- -Z blue
}
local PLASTIC = { 0.05, 0.05, 0.06 }

-- Face definitions: normal n, tangents u/v with cross(u, v) = n, and the
-- euler rotation that aims a +Z-facing quad at the normal.
local FACES = {
	{ key = "px", n = v3(1, 0, 0),  u = v3(0, 0, -1), v = v3(0, 1, 0),  rot = { 0, 90, 0 } },
	{ key = "nx", n = v3(-1, 0, 0), u = v3(0, 0, 1),  v = v3(0, 1, 0),  rot = { 0, -90, 0 } },
	{ key = "py", n = v3(0, 1, 0),  u = v3(1, 0, 0),  v = v3(0, 0, -1), rot = { -90, 0, 0 } },
	{ key = "ny", n = v3(0, -1, 0), u = v3(-1, 0, 0), v = v3(0, 0, -1), rot = { 90, 0, 0 } },
	{ key = "pz", n = v3(0, 0, 1),  u = v3(1, 0, 0),  v = v3(0, 1, 0),  rot = { 0, 0, 0 } },
	{ key = "nz", n = v3(0, 0, -1), u = v3(-1, 0, 0), v = v3(0, 1, 0),  rot = { 0, 180, 0 } },
}

-- Outer faces of a cubie at grid position (gx, gy, gz).
local function outerFaceKey(gx, gy, gz, face)
	if face.key == "px" then return gx == 1 end
	if face.key == "nx" then return gx == -1 end
	if face.key == "py" then return gy == 1 end
	if face.key == "ny" then return gy == -1 end
	if face.key == "pz" then return gz == 1 end
	if face.key == "nz" then return gz == -1 end
	return false
end

local function pushQuad(verts, indices, center, n, u, v, half, color, lift)
	local base = #verts
	local corners = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } }
	local o = vAdd(center, vScale(n, lift))

	for _, c in ipairs(corners) do
		local p = vAdd(o, vAdd(vScale(u, c[1] * half), vScale(v, c[2] * half)))
		verts[#verts + 1] = {
			p.x, p.y, p.z,
			n.x, n.y, n.z,
			(c[1] + 1) * 0.5, (c[2] + 1) * 0.5,
			color[1], color[2], color[3], 1.0,
		}
	end

	indices[#indices + 1] = base + 1
	indices[#indices + 1] = base + 2
	indices[#indices + 1] = base + 3
	indices[#indices + 1] = base + 1
	indices[#indices + 1] = base + 3
	indices[#indices + 1] = base + 4
end

-- One cubie = black plastic box + colored sticker quads on outer faces,
-- slightly raised for the classic bevelled-sticker look.
local function makeCubieMesh(gx, gy, gz)
	local verts, indices = {}, {}
	local h = CUBIE * 0.5

	for _, face in ipairs(FACES) do
		-- Box face (dark plastic).
		pushQuad(verts, indices, v3(0, 0, 0), face.n, face.u, face.v, h, PLASTIC, 0.0)

		-- Sticker quad on outer faces.
		if outerFaceKey(gx, gy, gz, face) then
			pushQuad(verts, indices, v3(0, 0, 0), face.n, face.u, face.v,
				h * STICKER, FACE_COLORS[face.key], STICKER_LIFT)
		end
	end

	return g3d.newMesh(verts, indices)
end

--------------------------------------------------------------------------
-- Game state
--------------------------------------------------------------------------

local cubies = {}       -- { gx, gy, gz, mesh, q }
local turn = nil        -- { axis, layer, dir, t, duration, fromUser }
local turnQueue = {}
local autoDemo = true
local autoTimer = 0.0

local camera = nil
local camYaw, camPitch, camDist = 35.0, 26.0, 7.5
local camVelYaw, camVelPitch = 0.0, 0.0
local orthoMode = false
local orthoHeight = 7.0

local sun, lamp1, lamp2, spot = nil, nil, nil, nil
local groundMesh, groundMaterial = nil, nil
local cubeMaterial = nil
local highlightMesh, highlightMaterial = nil
local gizmoMeshes = {}

local showcase = {}
local showcaseVisible = true

local moves = 0
local timerRunning = false
local timerStart = 0.0
local timerValue = 0.0
local solvedShown = false

local helpVisible = true
local skyMesh = nil
local drawCount = 0
local frameCount = 0

-- Mouse interaction state.
local drag = nil
-- drag = { mode = "orbit" | "turn", ... }
local hover = nil       -- { gx, gy, gz, faceIndex } or nil
local mouseX, mouseY = 0, 0

local AXES = { v3(1, 0, 0), v3(0, 1, 0), v3(0, 0, 1) }
local AXIS_NAMES = { "X", "Y", "Z" }

local FACE_TURNS = {
	u = { axis = 2, layer = 1, dir = -1 },
	d = { axis = 2, layer = -1, dir = 1 },
	r = { axis = 1, layer = 1, dir = -1 },
	l = { axis = 1, layer = -1, dir = 1 },
	f = { axis = 3, layer = 1, dir = -1 },
	b = { axis = 3, layer = -1, dir = 1 },
}

--------------------------------------------------------------------------
-- Camera helpers
--------------------------------------------------------------------------

local camPos = v3(0, 0, 0)

local function updateCameraBasis()
	local yaw = math.rad(camYaw)
	local pitch = math.rad(camPitch)
	local cp = math.cos(pitch)

	camPos = v3(
		camDist * cp * math.sin(yaw),
		camDist * math.sin(pitch),
		camDist * cp * math.cos(yaw))

	camera:setPosition(camPos.x, camPos.y, camPos.z)
	camera:lookAt(0.0, 0.0, 0.0)
end

local function applyProjectionMode()
	if orthoMode then
		camera:setProjectionMode("orthographic")
		local aspect = love.graphics.getWidth() / love.graphics.getHeight()
		local w = orthoHeight * aspect
		camera:setOrthoExtents(-w * 0.5, w * 0.5, -orthoHeight * 0.5, orthoHeight * 0.5)
	else
		camera:setProjectionMode("perspective")
	end
end

-- Project a world point to pixel coordinates; returns nil if behind camera.
local function projectToScreen(p)
	local w, h = love.graphics.getDimensions()
	local sx, sy, depth = camera:worldToScreen(p.x, p.y, p.z, w, h)
	if depth <= 0.02 then
		return nil
	end

	return sx, sy
end

-- Ray from the camera through a pixel (perspective or orthographic).
local function rayThroughPixel(mx, my)
	local w, h = love.graphics.getDimensions()
	local ox, oy, oz, dx, dy, dz = camera:screenRay(mx, my, w, h)
	return v3(ox, oy, oz), v3(dx, dy, dz)
end

--------------------------------------------------------------------------
-- Picking (ray vs axis-aligned cubie boxes)
--------------------------------------------------------------------------

-- Returns { cubieIndex, faceIndex, point } or nil.
local function pickCubie(mx, my)
	if turn ~= nil then
		return nil -- cubies in a turning layer are not axis-aligned
	end

	local origin, dir = rayThroughPixel(mx, my)
	local half = CUBIE * 0.5 + 0.02
	local bestT, bestIndex, bestAxis = math.huge, nil, -1

	for i, c in ipairs(cubies) do
		local center = v3(c.gx * CELL, c.gy * CELL, c.gz * CELL)
		local tmin, tmax, tminAxis = -math.huge, math.huge, -1
		local hit = true

		local o = { origin.x, origin.y, origin.z }
		local d = { dir.x, dir.y, dir.z }
		local cmin = { center.x - half, center.y - half, center.z - half }
		local cmax = { center.x + half, center.y + half, center.z + half }

		for k = 1, 3 do
			if math.abs(d[k]) < 1e-9 then
				if o[k] < cmin[k] or o[k] > cmax[k] then
					hit = false
					break
				end
			else
				local t1 = (cmin[k] - o[k]) / d[k]
				local t2 = (cmax[k] - o[k]) / d[k]
				if t1 > t2 then t1, t2 = t2, t1 end
				if t1 > tmin then
					tmin = t1
					tminAxis = k
				end
				if t2 < tmax then tmax = t2 end
				if tmin > tmax then
					hit = false
					break
				end
			end
		end

		if hit and tmin > 0.0 and tmin < bestT then
			bestT = tmin
			bestIndex = i
			bestAxis = tminAxis
		end
	end

	if bestIndex == nil then
		return nil
	end

	local point = vAdd(origin, vScale(dir, bestT))

	-- Entry face: the axis whose entry t won, on the side the ray came from.
	local d = { dir.x, dir.y, dir.z }
	local sign = d[bestAxis] > 0 and -1 or 1
	local normal = v3(0, 0, 0)
	if bestAxis == 1 then normal.x = sign
	elseif bestAxis == 2 then normal.y = sign
	else normal.z = sign end

	-- Which face definition matches this normal?
	local faceIndex = 0
	for fi, face in ipairs(FACES) do
		if vDot(face.n, normal) > 0.5 then
			faceIndex = fi
			break
		end
	end

	return { cubieIndex = bestIndex, faceIndex = faceIndex, point = point }
end

-- Choose the layer turn that best matches a screen-space drag on a face.
local function turnFromDrag(pick, startX, startY, endX, endY)
	local dragVec = v3(endX - startX, startY - endY, 0) -- screen y is down
	local dragLen = math.sqrt(dragVec.x * dragVec.x + dragVec.y * dragVec.y)
	if dragLen < 12.0 then
		return nil
	end
	dragVec = vScale(dragVec, 1.0 / dragLen)

	local c = cubies[pick.cubieIndex]
	local face = FACES[pick.faceIndex]
	if face == nil then
		return nil
	end

	-- Candidate turn axes: the two world axes perpendicular to the face.
	local bestScore, bestAxis = -1.0, 0
	local bestSign = 1.0

	for axis = 1, 3 do
		local ncomp = (axis == 1 and face.n.x) or (axis == 2 and face.n.y) or face.n.z
		if math.abs(ncomp) < 0.5 then
			local u = AXES[axis]
			-- Motion direction of the hit point under positive rotation.
			local rel = vSub(pick.point, v3(0, 0, 0))
			local motion = vCross(u, rel)
			local tip = vAdd(pick.point, vScale(motion, 0.5))
			local sx1, sy1 = projectToScreen(pick.point)
			local sx2, sy2 = projectToScreen(tip)

			if sx1 ~= nil and sx2 ~= nil then
				local sv = v3(sx2 - sx1, -(sy2 - sy1), 0)
				local svLen = math.sqrt(sv.x * sv.x + sv.y * sv.y)
				if svLen > 1e-4 then
					local score = math.abs(vDot(vScale(sv, 1.0 / svLen), dragVec))
					if score > bestScore then
						bestScore = score
						bestAxis = axis
						bestSign = vDot(sv, dragVec) > 0 and 1 or -1
					end
				end
			end
		end
	end

	if bestAxis == 0 then
		return nil
	end

	local layer = (bestAxis == 1 and c.gx) or (bestAxis == 2 and c.gy) or c.gz
	return { axis = bestAxis, layer = layer, dir = bestSign }
end

--------------------------------------------------------------------------
-- Turn logic
--------------------------------------------------------------------------

local function startTurn(axis, layer, dir, fromUser, duration)
	if turn ~= nil then
		return
	end
	turn = {
		axis = axis,
		layer = layer,
		dir = dir,
		t = 0.0,
		duration = duration or 0.28,
		fromUser = fromUser or false,
	}

	if fromUser and not timerRunning and not solvedShown then
		timerRunning = true
		timerStart = love.timer.getTime()
	end
end

local function queueRandomTurn()
	local axes = { 1, 2, 3 }
	local layers = { -1, 0, 1 }
	local dirs = { -1, 1 }
	turnQueue[#turnQueue + 1] = {
		axis = axes[love.math.random(3)],
		layer = layers[love.math.random(3)],
		dir = dirs[love.math.random(2)],
		fromUser = false,
	}
end

local function isSolved()
	for _, c in ipairs(cubies) do
		if c.gx ~= c.homeX or c.gy ~= c.homeY or c.gz ~= c.homeZ then
			return false
		end
		-- Centers may be twisted invisibly; edges and corners must be home.
		local nonzero = (c.homeX ~= 0 and 1 or 0) + (c.homeY ~= 0 and 1 or 0) + (c.homeZ ~= 0 and 1 or 0)
		if nonzero >= 2 then
			if math.abs(c.q[4]) < 0.999 then
				return false
			end
		end
	end
	return true
end

local function finishTurn()
	local ax = AXES[turn.axis]
	local q90 = quatFromAxisAngle(ax.x, ax.y, ax.z, math.pi * 0.5 * turn.dir)

	for _, c in ipairs(cubies) do
		local coord = (turn.axis == 1 and c.gx) or (turn.axis == 2 and c.gy) or c.gz
		if coord == turn.layer then
			local nx, ny, nz = quatRotate(q90, c.gx, c.gy, c.gz)
			c.gx = math.floor(nx + 0.5)
			c.gy = math.floor(ny + 0.5)
			c.gz = math.floor(nz + 0.5)
			c.q = quatSnapAxisAligned(quatMul(q90, c.q))
		end
	end

	moves = moves + 1

	if timerRunning and isSolved() then
		timerRunning = false
		timerValue = love.timer.getTime() - timerStart
		solvedShown = true
	end

	turn = nil
end

local function resetCube()
	turn = nil
	turnQueue = {}
	moves = 0
	timerRunning = false
	timerValue = 0.0
	solvedShown = false
	for _, c in ipairs(cubies) do
		c.gx, c.gy, c.gz = c.homeX, c.homeY, c.homeZ
		c.q = quatIdentity()
	end
end

--------------------------------------------------------------------------
-- Scene construction
--------------------------------------------------------------------------

local function makeGround()
	-- Custom mesh with world-scaled UVs so the checker texture tiles.
	local size, tiles = 30.0, 10.0
	local half = size * 0.5
	local y = -1.65
	local s = size / tiles

	-- Wound CCW when viewed from above (+Y), matching the module's
	-- front-face convention.
	local verts = {
		{ -half, y, -half, 0, 1, 0, 0, 0, 1, 1, 1, 1 },
		{ -half, y, half, 0, 1, 0, 0, s, 1, 1, 1, 1 },
		{ half, y, half, 0, 1, 0, s, s, 1, 1, 1, 1 },
		{ half, y, -half, 0, 1, 0, s, 0, 1, 1, 1, 1 },
	}
	groundMesh = g3d.newMesh(verts, { 1, 2, 3, 1, 3, 4 })

	local imageData = love.image.newImageData(2, 2)
	imageData:setPixel(0, 0, 0.82, 0.80, 0.77, 1)
	imageData:setPixel(1, 0, 0.52, 0.51, 0.49, 1)
	imageData:setPixel(0, 1, 0.52, 0.51, 0.49, 1)
	imageData:setPixel(1, 1, 0.82, 0.80, 0.77, 1)

	local texture = love.graphics.newImage(imageData)
	texture:setFilter("nearest", "nearest")
	texture:setWrap("repeat", "repeat")

	groundMaterial = g3d.newMaterial()
	groundMaterial:setDiffuseTexture(texture)
	groundMaterial:setSpecularParams(0.08, 8.0)
	groundMesh:setMaterial(groundMaterial)
end

local function makeShowcase()
	local defs = {
		{ mesh = g3d.newTorus(32, 16, 0.55, 0.18), color = { 0.85, 0.65, 0.20 }, spec = { 0.9, 64.0 } },
		{ mesh = g3d.newSphere(16, 24, 0.5), color = { 0.75, 0.78, 0.82 }, spec = { 1.0, 96.0 } },
		{ mesh = g3d.newCapsule(24, 8, 0.35, 0.8), color = { 0.80, 0.15, 0.15 }, spec = { 0.1, 8.0 } },
		{ mesh = g3d.newCone(24, 0.5, 1.0), color = { 0.15, 0.65, 0.25 }, spec = { 0.3, 16.0 } },
		{ mesh = g3d.newCylinder(24, 0.45, 1.0), color = { 0.20, 0.40, 0.85 }, spec = { 0.4, 24.0 } },
		{ mesh = g3d.newBox(0.9, 0.9, 0.9), color = { 0.55, 0.15, 0.55 }, spec = { 0.2, 12.0 }, emissive = { 0.45, 0.08, 0.45 } },
	}

	for i, def in ipairs(defs) do
		local material = g3d.newMaterial()
		material:setColor(def.color[1], def.color[2], def.color[3], 1.0)
		material:setSpecularParams(def.spec[1], def.spec[2])
		if def.emissive then
			material:setEmissiveColor(def.emissive[1], def.emissive[2], def.emissive[3])
		end
		def.mesh:setMaterial(material)
		def.phase = (i - 1) * math.pi * 2.0 / #defs
		showcase[#showcase + 1] = def
	end
end

local function makeLights()
	sun = g3d.newLight("directional")
	sun:setDirection(0.4, -1.0, 0.3)
	sun:setColor(1.0, 0.98, 0.92)
	sun:setIntensity(1.1)

	lamp1 = g3d.newLight("point")
	lamp1:setColor(1.0, 0.55, 0.30)
	lamp1:setIntensity(2.2)
	lamp1:setRange(11.0)
	lamp1:setFalloff(1.2)

	lamp2 = g3d.newLight("point")
	lamp2:setColor(0.30, 0.55, 1.0)
	lamp2:setIntensity(1.8)
	lamp2:setRange(11.0)
	lamp2:setFalloff(1.2)

	spot = g3d.newLight("spot")
	spot:setPosition(0.0, 7.5, 0.0)
	spot:setDirection(0.0, -1.0, 0.0)
	spot:setColor(1.0, 1.0, 1.0)
	spot:setIntensity(2.0)
	spot:setRange(20.0)
	spot:setSpotAngles(14.0, 32.0)
	spot:setFalloff(1.0)

	g3d.setAmbientColor(0.16, 0.16, 0.20)
	g3d.addLight(sun)
	g3d.addLight(lamp1)
	g3d.addLight(lamp2)
	g3d.addLight(spot)
end

--------------------------------------------------------------------------
-- LOVE callbacks
--------------------------------------------------------------------------

function love.load()
	print("L3D: load start")
	love.math.setRandomSeed(os.time())

	camera = g3d.newCamera()
	camera:setFOV(60.0)
	camera:setNear(0.1)
	camera:setFar(100.0)
	camera:setAspect(love.graphics.getWidth() / love.graphics.getHeight())
	g3d.setCamera(camera)
	applyProjectionMode()
	updateCameraBasis()
	print("L3D: camera ok")

	-- Shared cubie material (vertex colors do the tinting).
	cubeMaterial = g3d.newMaterial()
	cubeMaterial:setColor(1.0, 1.0, 1.0, 1.0)
	cubeMaterial:setSpecularParams(0.35, 24.0)
	cubeMaterial:setCullMode("back")

	for gx = -1, 1 do
		for gy = -1, 1 do
			for gz = -1, 1 do
				local mesh = makeCubieMesh(gx, gy, gz)
				mesh:setMaterial(cubeMaterial)
				cubies[#cubies + 1] = {
					gx = gx, gy = gy, gz = gz,
					homeX = gx, homeY = gy, homeZ = gz,
					mesh = mesh,
					q = quatIdentity(),
				}
			end
		end
	end
	print("L3D: cubies ok")

	makeGround()
	print("L3D: ground ok")
	makeShowcase()
	print("L3D: showcase ok")
	makeLights()
	print("L3D: lights ok")

	-- Face-highlight quad (+Z facing, scaled/rotated per face at draw time).
	-- Wound CCW when viewed from +Z.
	highlightMesh = g3d.newMesh({
		{ -0.5, -0.5, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1 },
		{ 0.5, -0.5, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1 },
		{ 0.5, 0.5, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
		{ -0.5, 0.5, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1 },
	}, { 1, 2, 3, 1, 3, 4 })
	highlightMaterial = g3d.newMaterial()
	highlightMaterial:setUnlit(true)
	highlightMaterial:setColor(1.0, 1.0, 1.0, 0.30)
	highlightMesh:setMaterial(highlightMaterial)

	-- Little glowing spheres marking the orbiting point lights (one mesh
	-- per color; material comes from the mesh, not the draw call).
	local gizmoDefs = {
		{ color = { 1.0, 0.60, 0.30 } },
		{ color = { 0.35, 0.60, 1.0 } },
	}
	gizmoMeshes = {}
	for _, def in ipairs(gizmoDefs) do
		local mesh = g3d.newSphere(10, 14, 0.12)
		local m = g3d.newMaterial()
		m:setUnlit(true)
		m:setColor(def.color[1], def.color[2], def.color[3], 1.0)
		mesh:setMaterial(m)
		gizmoMeshes[#gizmoMeshes + 1] = mesh
	end

	-- Sky gradient (2D mesh drawn before the 3D pass).
	local w, hgt = love.graphics.getDimensions()
	skyMesh = love.graphics.newMesh({
		{ 0, 0, 0, 0, 0.02, 0.04, 0.12, 1 },
		{ w, 0, 0, 0, 0.02, 0.04, 0.12, 1 },
		{ w, hgt, 0, 0, 0.45, 0.34, 0.30, 1 },
		{ 0, hgt, 0, 0, 0.45, 0.34, 0.30, 1 },
	}, "fan")
	print("L3D: load complete")
end

function love.update(dt)
	frameCount = frameCount + 1

	-- Debug: capture screenshots at 1s and 5s for headless diagnosis.
	if frameCount == 60 then
		love.graphics.captureScreenshot("shot1.png")
	elseif frameCount == 300 then
		love.graphics.captureScreenshot("shot5.png")
	end

	-- Slow automatic camera drift while the auto-demo runs.
	if autoDemo and drag == nil then
		camYaw = camYaw + dt * 9.0
	end

	-- Camera inertia.
	if drag == nil or drag.mode ~= "orbit" then
		if math.abs(camVelYaw) > 0.01 or math.abs(camVelPitch) > 0.01 then
			camYaw = camYaw + camVelYaw * dt
			camPitch = math.max(-85.0, math.min(85.0, camPitch + camVelPitch * dt))
			local decay = math.exp(-4.5 * dt)
			camVelYaw = camVelYaw * decay
			camVelPitch = camVelPitch * decay
		end
	end
	updateCameraBasis()

	-- Orbiting colored point lights (gizmos follow).
	local t = love.timer.getTime()
	local p1 = v3(3.2 * math.cos(t * 0.9), 2.6, 3.2 * math.sin(t * 0.9))
	local p2 = v3(3.2 * math.cos(t * 0.9 + math.pi), -1.2, 3.2 * math.sin(t * 0.9 + math.pi))
	lamp1:setPosition(p1.x, p1.y, p1.z)
	lamp2:setPosition(p2.x, p2.y, p2.z)

	-- Turn animation with ease-out.
	if turn ~= nil then
		turn.t = turn.t + dt / turn.duration
		if turn.t >= 1.0 then
			finishTurn()
		end
	end

	-- Queued turns and the auto-demo.
	if turn == nil and #turnQueue > 0 then
		local next_ = table.remove(turnQueue, 1)
		startTurn(next_.axis, next_.layer, next_.dir, next_.fromUser, 0.16)
	elseif turn == nil and autoDemo then
		autoTimer = autoTimer + dt
		if autoTimer >= 1.4 then
			autoTimer = 0.0
			queueRandomTurn()
		end
	end

	-- Hover highlight (only when the scene is at rest).
	if turn == nil and (drag == nil or drag.mode ~= "orbit") then
		local pick = pickCubie(mouseX, mouseY)
		if pick ~= nil then
			hover = { cubieIndex = pick.cubieIndex, faceIndex = pick.faceIndex }
		else
			hover = nil
		end
	else
		hover = nil
	end

	if timerRunning then
		timerValue = love.timer.getTime() - timerStart
	end
end

function love.mousepressed(x, y, button)
	if button ~= 1 and button ~= 2 then
		return
	end

	local pick = nil
	if button == 1 then
		pick = pickCubie(x, y)
	end

	if pick ~= nil then
		drag = { mode = "turn", startX = x, startY = y, pick = pick }
	else
		drag = { mode = "orbit", lastX = x, lastY = y }
		camVelYaw, camVelPitch = 0.0, 0.0
	end
end

function love.mousemoved(x, y, dx, dy)
	mouseX, mouseY = x, y

	if drag == nil then
		return
	end

	if drag.mode == "orbit" then
		local sens = 0.30
		camYaw = camYaw - dx * sens
		camPitch = math.max(-85.0, math.min(85.0, camPitch + dy * sens))
		camVelYaw = -dx * sens * 60.0
		camVelPitch = dy * sens * 60.0
	elseif drag.mode == "turn" then
		local result = turnFromDrag(drag.pick, drag.startX, drag.startY, x, y)
		if result ~= nil then
			solvedShown = false
			startTurn(result.axis, result.layer, result.dir, true)
			drag = nil
		end
	end
end

function love.mousereleased(x, y, button)
	drag = nil
end

function love.wheelmoved(dx, dy)
	if dy > 0 then
		camDist = math.max(3.5, camDist * 0.88)
	else
		camDist = math.min(20.0, camDist * 1.12)
	end
end

function love.keypressed(key, scancode, isrepeat)
	if isrepeat then
		return
	end

	if key == "escape" then
		love.event.quit()
		return
	end

	if key == "return" or key == "kpenter" then
		autoDemo = not autoDemo
		autoTimer = 0.0
		return
	end

	if key == "s" then
		solvedShown = false
		timerRunning = false
		timerValue = 0.0
		moves = 0
		for i = 1, 25 do
			queueRandomTurn()
		end
		return
	end

	if key == "r" then
		resetCube()
		return
	end

	if key == "space" then
		queueRandomTurn()
		return
	end

	if key == "o" then
		orthoMode = not orthoMode
		applyProjectionMode()
		return
	end

	if key == "g" then
		showcaseVisible = not showcaseVisible
		return
	end

	if key == "c" then
		camYaw, camPitch, camDist = 35.0, 26.0, 7.5
		camVelYaw, camVelPitch = 0.0, 0.0
		return
	end

	if key == "h" then
		helpVisible = not helpVisible
		return
	end

	local face = FACE_TURNS[key]
	if face then
		local dir = face.dir
		if love.keyboard.isDown("lshift") or love.keyboard.isDown("rshift") then
			dir = -dir
		end
		solvedShown = false
		startTurn(face.axis, face.layer, dir, true)
	end
end

function love.resize(w, h)
	camera:setAspect(w / h)
	applyProjectionMode()

	-- Rebuild the sky mesh for the new size.
	skyMesh = love.graphics.newMesh({
		{ 0, 0, 0, 0, 0.02, 0.04, 0.12, 1 },
		{ w, 0, 0, 0, 0.02, 0.04, 0.12, 1 },
		{ w, h, 0, 0, 0.45, 0.34, 0.30, 1 },
		{ 0, h, 0, 0, 0.45, 0.34, 0.30, 1 },
	}, "fan")
end

local function drawCube()
	for i, c in ipairs(cubies) do
		local x, y, z = c.gx * CELL, c.gy * CELL, c.gz * CELL
		local rx, ry, rz

		if turn ~= nil then
			local coord = (turn.axis == 1 and c.gx) or (turn.axis == 2 and c.gy) or c.gz
			if coord == turn.layer then
				local ax = AXES[turn.axis]
				local angle = math.pi * 0.5 * turn.dir * easeOutCubic(turn.t)
				local qt = quatFromAxisAngle(ax.x, ax.y, ax.z, angle)
				x, y, z = quatRotate(qt, x, y, z)
				rx, ry, rz = quatToEulerDegrees(quatMul(qt, c.q))
			else
				rx, ry, rz = quatToEulerDegrees(c.q)
			end
		else
			rx, ry, rz = quatToEulerDegrees(c.q)
		end

		g3d.draw(c.mesh, x, y, z, rx, ry, rz)
	end
end

local function drawShowcase(t)
	if not showcaseVisible then
		return
	end

	for _, def in ipairs(showcase) do
		local a = t * 0.25 + def.phase
		local x = math.cos(a) * 5.2
		local z = math.sin(a) * 5.2
		local y = 1.15 + math.sin(t * 0.8 + def.phase) * 0.35
		local ry = math.deg(t * 0.6) + math.deg(def.phase)
		local rx = math.deg(math.sin(t * 0.4 + def.phase) * 0.5)
		g3d.draw(def.mesh, x, y, z, rx, ry, 0)
	end
end

local function drawLightGizmos(t)
	local p1x = 3.2 * math.cos(t * 0.9)
	local p1z = 3.2 * math.sin(t * 0.9)
	local p2x = 3.2 * math.cos(t * 0.9 + math.pi)
	local p2z = 3.2 * math.sin(t * 0.9 + math.pi)

	g3d.draw(gizmoMeshes[1], p1x, 2.6, p1z, 0, 0, 0, 1, 1, 1)
	g3d.draw(gizmoMeshes[2], p2x, -1.2, p2z, 0, 0, 0, 1, 1, 1)
end

local function drawHighlight()
	if hover == nil or turn ~= nil then
		return
	end

	local c = cubies[hover.cubieIndex]
	local face = FACES[hover.faceIndex]
	if c == nil or face == nil then
		return
	end

	local lift = CUBIE * 0.5 + STICKER_LIFT + 0.004
	local x = c.gx * CELL + face.n.x * lift
	local y = c.gy * CELL + face.n.y * lift
	local z = c.gz * CELL + face.n.z * lift
	local s = CUBIE * STICKER * 0.94

	g3d.draw(highlightMesh, x, y, z, face.rot[1], face.rot[2], face.rot[3], s, s, 1)
end

function love.draw()
	local w, h = love.graphics.getDimensions()

	-- Sky gradient (2D, before the 3D pass; begin3d only clears depth).
	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.draw(skyMesh)

	-- 3D pass.
	drawCount = drawCount + 1
	if drawCount <= 3 then print("L3D: frame " .. drawCount .. " begin3d...") end
	g3d.begin3d()
	if drawCount <= 3 then print("L3D: frame " .. drawCount .. " scene queued") end

	local t = love.timer.getTime()
	drawCube()
	g3d.draw(groundMesh, 0, 0, 0)
	drawShowcase(t)
	drawLightGizmos(t)
	drawHighlight()

	g3d.end3d()
	if drawCount <= 3 then print("L3D: frame " .. drawCount .. " end3d ok") end
	local stats = g3d.getStats()
	local fps = love.timer.getFPS()

	love.graphics.setColor(0, 0, 0, 0.45)
	love.graphics.rectangle("fill", 10, 10, 320, 58)
	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.print("LOVING3D benchmark - Rubik's Cube", 18, 16)
	love.graphics.print(string.format("FPS: %d   draws queued: %d   flushed: %d   lights: 4/%d",
		fps, stats.drawsPassed, stats.drawsFlushed, stats.maxLights), 18, 34)

	-- Timer / moves panel.
	local stateText = solvedShown and "SOLVED!" or (timerRunning and "SOLVING" or "READY")
	local stateColor = solvedShown and { 0.3, 1.0, 0.4 } or { 1, 1, 1 }
	love.graphics.setColor(0, 0, 0, 0.45)
	love.graphics.rectangle("fill", w - 220, 10, 210, 58)
	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.print(string.format("time  %6.1f s", timerValue), w - 212, 16)
	love.graphics.print(string.format("moves %d   [%s]", moves, stateText), w - 212, 34)

	if helpVisible then
		local lines = {
			"drag cube: turn layer    drag space: orbit    wheel: zoom",
			"U/D/L/R/F/B faces (+shift)   Space random   S scramble   R reset",
			"Enter auto-demo   O ortho   G showcase   C camera   H help",
		}
		love.graphics.setColor(0, 0, 0, 0.45)
		love.graphics.rectangle("fill", 10, h - 78, 470, 68)
		love.graphics.setColor(1, 1, 1, 1)
		for i, line in ipairs(lines) do
			love.graphics.print(line, 18, h - 70 + (i - 1) * 20)
		end
	end

	if solvedShown then
		love.graphics.setColor(0.1, 0.9, 0.3, 0.9)
		love.graphics.print("SOLVED!", w * 0.5 - 40, h * 0.5 - 40)
	end
end
