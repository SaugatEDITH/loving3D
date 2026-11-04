local g3d = love.graphics3d

local scene
local camera
local cube
local sphere
local angle = 0

function love.load()
	scene = love.graphics.newCanvas(640, 480, { format = "rgba8", readable = true })
	scene:setFilter("linear", "linear")

	camera = g3d.newCamera()
	camera:setAspect(scene:getWidth() / scene:getHeight())
	camera:setPosition(3, 2.5, 6)
	camera:lookAt(0, 0, 0)
	g3d.setCamera(camera)

	local cubeMaterial = g3d.newMaterial()
	cubeMaterial:setColor(0.15, 0.65, 1, 1)
	cubeMaterial:setSpecularParams(0.35, 40)
	cube = g3d.newCube()
	cube:setMaterial(cubeMaterial)

	local sphereMaterial = g3d.newMaterial()
	sphereMaterial:setColor(1, 0.45, 0.12, 1)
	sphereMaterial:setUnlit(true)
	sphere = g3d.newSphere(16, 24, 0.75)
	sphere:setMaterial(sphereMaterial)
	print("3D Canvas demo ready")
end

function love.update(dt)
	angle = angle + dt * 30
end

function love.draw()
	scene:renderTo(function()
		love.graphics.clear(0.035, 0.055, 0.1, 1)
		g3d.begin3d()
		g3d.draw(cube, -0.9, 0, 0, angle, angle * 0.7, 0)
		g3d.draw(sphere, 1.1, 0, 0)
		g3d.end3d()
	end)

	love.graphics.clear(0.04, 0.045, 0.065, 1)
	local scale = math.min((love.graphics.getWidth() - 64) / scene:getWidth(),
		(love.graphics.getHeight() - 100) / scene:getHeight())
	local width, height = scene:getWidth() * scale, scene:getHeight() * scale
	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.draw(scene, (love.graphics.getWidth() - width) * 0.5,
		(love.graphics.getHeight() - height) * 0.5, 0, scale, scale)
	love.graphics.print("3D rendered to a readable Canvas, then composited as 2D.", 20, 18)
end
