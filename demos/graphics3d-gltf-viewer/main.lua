local g3d = love.graphics3d

local camera
local model
local modelPath = "assets/Box.glb"
local radius = 5
local yaw = 0.6
local pitch = 0.25
local dragging = false
local lastX, lastY = 0, 0
local loadError
local editingPath = false
local pathBuffer = ""

local function updateCamera()
	camera:setPosition(
		radius * math.cos(pitch) * math.sin(yaw),
		radius * math.sin(pitch),
		radius * math.cos(pitch) * math.cos(yaw)
	)
	camera:lookAt(0, 0, 0)
end

local function loadModel(path)
	local ok, result = pcall(g3d.newModelFromFile, path)
	if ok then
		model = result
		modelPath = path
		loadError = nil
	else
		loadError = tostring(result)
	end
end

function love.load(args)
	camera = g3d.newCamera()
	camera:setAspect(love.graphics.getWidth() / love.graphics.getHeight())
	g3d.setCamera(camera)

	local light = g3d.newLight("directional")
	light:setDirection(-0.5, -1, -0.6)
	light:setColor(1, 0.94, 0.82)
	light:setIntensity(1.2)
	g3d.clearLights()
	g3d.addLight(light)
	g3d.setAmbientColor(0.25, 0.27, 0.32)

	updateCamera()
	loadModel(args and args[1] or modelPath)
end

function love.resize(width, height)
	camera:setAspect(width / math.max(height, 1))
end

function love.update()
	updateCamera()
end

function love.draw()
	love.graphics.clear(0.055, 0.07, 0.1, 1)
	if model then
		g3d.begin3d()
		g3d.draw(model, 0, 0, 0)
		g3d.end3d()
	end

	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.print("glTF 2.0 / GLB viewer", 14, 14)
	love.graphics.print("Model: " .. modelPath, 14, 36)
	love.graphics.print("Drag to orbit | Wheel to zoom | R reset | Enter type/load a path", 14, 58)
	if editingPath then love.graphics.print("Path: " .. pathBuffer .. "_", 14, 80) end
	if loadError then
		love.graphics.setColor(1, 0.35, 0.3, 1)
		love.graphics.printf(loadError, 14, editingPath and 108 or 86, love.graphics.getWidth() - 28)
	end
end

function love.mousepressed(x, y, button)
	if button == 1 then
		dragging = true
		lastX, lastY = x, y
	end
end

function love.mousereleased(_, _, button)
	if button == 1 then dragging = false end
end

function love.mousemoved(x, y)
	if not dragging then return end
	yaw = yaw - (x - lastX) * 0.008
	pitch = math.max(-1.45, math.min(1.45, pitch + (y - lastY) * 0.008))
	lastX, lastY = x, y
end

function love.wheelmoved(_, amount)
	radius = math.max(1, math.min(100, radius * math.exp(-amount * 0.12)))
	updateCamera()
end

function love.keypressed(key)
	if key == "r" then
		radius, yaw, pitch = 5, 0.6, 0.25
		updateCamera()
	elseif key == "return" then
		if editingPath then
			editingPath = false
			if pathBuffer ~= "" then loadModel(pathBuffer) end
		else
			editingPath = true
			pathBuffer = modelPath
		end
	elseif key == "backspace" and editingPath then
		pathBuffer = pathBuffer:sub(1, -2)
	elseif key == "escape" then
		editingPath = false
	end
end

function love.textinput(text)
	if editingPath then pathBuffer = pathBuffer .. text end
end
