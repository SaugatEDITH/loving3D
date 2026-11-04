local g3d = love.graphics3d
local objectCount = 5000
local mesh
local camera
local instances = {}
local mode = "draw"
local benchmark = false
local frameSamples = 0
local frameTimeTotal = 0
local frameTimeMin = math.huge
local frameTimeMax = 0
local benchmarkPhase = 1
local lastStats = { drawCalls = 0, triangles = 0 }
local modes = {
	{ name = "Individual draws", draw = "draw", batching = false, far = 180 },
	{ name = "Automatic batching", draw = "draw", batching = true, far = 180 },
	{ name = "GPU instancing", draw = "instanced", batching = false, far = 180 },
	{ name = "Frustum culling", draw = "draw", batching = false, far = 60 },
}

local function resetSamples()
	frameSamples = 0
	frameTimeTotal = 0
	frameTimeMin = math.huge
	frameTimeMax = 0
end

local function setMode(index)
	benchmarkPhase = index
	local selected = modes[index]
	mode = selected.draw
	g3d.setAutoBatching(selected.batching)
	camera:setFar(selected.far)
	resetSamples()
end

function love.load(args)
	for _, arg in ipairs(args or {}) do
		if arg == "--benchmark" then
			benchmark = true
		end
	end
	if benchmark then
		love.window.setMode(1920, 1080, { depth = true, resizable = false })
	end

	camera = g3d.newCamera()
	camera:setPosition(0, 27, 38)
	camera:setAspect(love.graphics.getWidth() / love.graphics.getHeight())
	camera:setFOV(62)
	camera:setFar(180)
	camera:lookAt(0, 0, -32)
	g3d.setCamera(camera)
	g3d.setAmbientColor(0.35, 0.35, 0.4)
	g3d.setAutoBatching(false)

	local sun = g3d.newLight("directional")
	sun:setDirection(-0.4, -1, -0.5)
	sun:setIntensity(1.2)
	g3d.addLight(sun)

	local material = g3d.newMaterial()
	material:setColor(0.2, 0.65, 0.95, 1)
	material:setUnlit(true)
	mesh = g3d.newBox(0.58, 0.58, 0.58)
	mesh:setMaterial(material)

	for i = 1, objectCount do
		local column = (i - 1) % 100
		local row = math.floor((i - 1) / 100)
		local x = (column - 49.5) * 0.78
		local y = 0.45 + ((column * 13 + row * 7) % 7) * 0.22
		local z = -row * 1.35
		instances[i] = { x, y, z, 0, (i * 17) % 360, 0, 1, 1, 1 }
	end

	print(string.format("LOVING3D stress scene ready: %d objects.", objectCount))
	local renderer, version, vendor, device = love.graphics.getRendererInfo()
	print(string.format("Renderer: %s %s | %s | %s", renderer or "unknown",
		version or "unknown", vendor or "unknown", device or "unknown"))
	print("Space: cycle benchmark modes; --benchmark runs 600 frames in each mode.")
end

function love.update(dt)
	frameSamples = frameSamples + 1
	frameTimeTotal = frameTimeTotal + dt
	frameTimeMin = math.min(frameTimeMin, dt)
	frameTimeMax = math.max(frameTimeMax, dt)
	if frameSamples < 600 then
		return
	end

	local stats = g3d.getStats()
	local averageMs = frameTimeTotal * 1000 / frameSamples
	lastStats = stats
	print(string.format("%s: %d frames, avg/min/max %.3f/%.3f/%.3f ms, drawCalls=%d, triangles=%d, queued=%d, culled=%d",
		modes[benchmarkPhase].name, frameSamples, averageMs, frameTimeMin * 1000,
		frameTimeMax * 1000, stats.drawCalls, stats.triangles, stats.queued, stats.culled))
	resetSamples()

	if benchmark then
		if benchmarkPhase < #modes then
			setMode(benchmarkPhase + 1)
		else
			love.event.quit(0)
		end
	end
end

function love.draw()
	love.graphics.clear(0.025, 0.035, 0.055, 1)

	g3d.begin3d()
	if mode == "draw" then
		for i = 1, objectCount do
			local transform = instances[i]
			g3d.draw(mesh, transform[1], transform[2], transform[3], 0, transform[5], 0)
		end
	else
		g3d.drawInstanced(mesh, instances)
	end
	g3d.end3d()

	lastStats = g3d.getStats()
	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.print(string.format("LOVING3D | %d objects | %s", objectCount,
		modes[benchmarkPhase].name), 16, 16)
	love.graphics.print(string.format("Draw calls: %d   triangles: %d   queued: %d   FPS: %d",
		lastStats.drawCalls, lastStats.triangles, lastStats.queued, love.timer.getFPS()), 16, 38)
	love.graphics.print("Space: cycle mode   Esc: quit   Benchmark: 600 frames per mode", 16, 60)
end

function love.keypressed(key)
	if key == "escape" then
		love.event.quit()
	elseif key == "space" then
		setMode(benchmarkPhase % #modes + 1)
	end
end

function love.resize(width, height)
	if camera and height > 0 then
		camera:setAspect(width / height)
	end
end
