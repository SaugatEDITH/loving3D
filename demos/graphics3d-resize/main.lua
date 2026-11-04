local g3d = love.graphics3d

local camera
local cube
local angle = 0
local showBounds = false
local showNormals = false

function love.load()
	camera = g3d.newCamera()
	camera:setPosition(3, 2.5, 6)
	camera:lookAt(0, 0, 0)
	camera:setAspect(love.graphics.getWidth() / love.graphics.getHeight())
	g3d.setCamera(camera)

	local material = g3d.newMaterial()
	material:setColor(0.3, 0.8, 0.55, 1)
	material:setSpecularParams(0.3, 36)
	cube = g3d.newCube()
	cube:setMaterial(material)
	print("Resize/fullscreen demo ready")
end

function love.update(dt)
	angle = angle + dt * 30
end

function love.draw()
	love.graphics.clear(0.035, 0.045, 0.07, 1)
	g3d.begin3d()
	g3d.draw(cube, 0, 0, 0, angle * 0.6, angle, 0, 1.5, 1.5, 1.5)
	if showBounds then
		g3d.debugBox(cube, 0, 0, 0, angle * 0.6, angle, 0, 1.5, 1.5, 1.5)
	end
	if showNormals then
		g3d.debugNormals(cube, 0, 0, 0, angle * 0.6, angle, 0, 1.5, 1.5, 1.5, 0.3)
	end
	g3d.end3d()
	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.print("Resize the window or press F to toggle fullscreen.", 18, 18)
	love.graphics.print(string.format("%d x %d   B: bounds %s   N: normals %s",
		love.graphics.getWidth(), love.graphics.getHeight(),
		showBounds and "on" or "off", showNormals and "on" or "off"), 18, 40)
end

function love.resize(width, height)
	if camera ~= nil and height > 0 then
		camera:setAspect(width / height)
	end
end

function love.keypressed(key)
	if key == "escape" then
		love.event.quit()
	elseif key == "f" then
		love.window.setFullscreen(not love.window.getFullscreen())
	elseif key == "b" then
		showBounds = not showBounds
	elseif key == "n" then
		showNormals = not showNormals
	end
end
