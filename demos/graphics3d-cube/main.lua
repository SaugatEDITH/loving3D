--
-- LOVING3D - minimal interactive 3D rendering demo.
--
-- Copyright (c) 2026 Saugat Pokharel
--
-- This file is part of LOVING3D, an independent extension adding native 3D
-- capabilities to the LOVE game engine (love2d.org). The underlying engine
-- remains freely distributable under the terms of its original zlib license
-- (see license.txt).
--

local g3d = love.graphics3d

local cube
local camera
local rotationX = 20
local rotationY = 25
local frameCount = 0

function love.load()
	camera = g3d.newCamera()
	camera:setPosition(4, 3, 6)
	camera:setAspect(love.graphics.getWidth() / love.graphics.getHeight())
	camera:setFOV(55)
	camera:lookAt(0, 0, 0)
	g3d.setCamera(camera)

	g3d.setAmbientColor(0.12, 0.12, 0.16)

	local light = g3d.newLight("directional")
	light:setDirection(-0.5, -1, -0.7)
	light:setColor(1, 0.92, 0.78)
	light:setIntensity(2)
	g3d.addLight(light)

	local material = g3d.newMaterial()
	material:setColor(0.2, 0.65, 0.95, 1)
	material:setSpecularParams(0.35, 40)

	cube = g3d.newCube()
	cube:setMaterial(material)

	print("3D cube demo ready")
end

function love.update(dt)
	rotationY = rotationY + 24 * dt
	frameCount = frameCount + 1

	if frameCount % 120 == 0 then
		print("3D cube rendered " .. frameCount .. " frames")
	end

	love.timer.sleep(math.max(0, 1 / 60 - dt))
end

function love.draw()
	love.graphics.clear(0.035, 0.045, 0.07, 1)

	g3d.begin3d()
	g3d.draw(cube, 0, 0, 0, rotationX, rotationY, 0, 1.6, 1.6, 1.6)
	g3d.end3d()

	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.print("LOVING3D | rotating lit cube", 16, 16)
	love.graphics.print("Arrow keys: rotate   Esc: quit", 16, 38)
end

function love.keypressed(key)
	if key == "escape" then
		love.event.quit()
	elseif key == "up" then
		rotationX = rotationX - 10
	elseif key == "down" then
		rotationX = rotationX + 10
	elseif key == "left" then
		rotationY = rotationY - 10
	elseif key == "right" then
		rotationY = rotationY + 10
	end
end

function love.resize(width, height)
	if camera ~= nil and height > 0 then
		camera:setAspect(width / height)
	end
end
