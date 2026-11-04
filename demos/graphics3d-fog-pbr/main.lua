local g3d = love.graphics3d
local camera
local spheres = {}
local benchmark = false
local phase = 1
local samples = 0
local total = 0
local minFrame = math.huge
local maxFrame = 0
local phases = {
	{ name = "Blinn-Phong baseline", fog = "none", pbr = false },
	{ name = "Linear fog", fog = "linear", pbr = false },
	{ name = "Exponential fog", fog = "exp", pbr = false },
	{ name = "Metallic-roughness PBR", fog = "none", pbr = true },
}

local function configure(index)
	phase = index
	local selected = phases[index]
	g3d.setFog(selected.fog, 0.18, 0.24, 0.32, 0.035, 4, 24)
	for _, sphere in ipairs(spheres) do
		sphere.material:setPBR(selected.pbr)
	end
	samples = 0
	total = 0
	minFrame = math.huge
	maxFrame = 0
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
	camera:setPosition(0, 5, 14)
	camera:setAspect(love.graphics.getWidth() / love.graphics.getHeight())
	camera:setFOV(55)
	camera:lookAt(0, 1, 0)
	g3d.setCamera(camera)
	g3d.setAmbientColor(0.12, 0.12, 0.12)

	local sun = g3d.newLight("directional")
	sun:setDirection(-0.4, -1, -0.5)
	sun:setIntensity(2)
	g3d.addLight(sun)

	local sphereMesh = g3d.newSphere(16, 24, 0.7)
	for row = 1, 5 do
		for column = 1, 5 do
			local material = g3d.newMaterial()
			material:setColor(0.78, 0.48 + row * 0.07, 0.2 + column * 0.06, 1)
			material:setMetallic((column - 1) / 4)
			material:setRoughness(0.12 + (row - 1) * 0.2)
			spheres[#spheres + 1] = {
				mesh = sphereMesh,
				material = material,
				x = (column - 3) * 2.0,
				y = 0.8,
				z = -(row - 1) * 2.0,
			}
		end
	end
	configure(1)

	local renderer, version, vendor, device = love.graphics.getRendererInfo()
	print(string.format("Renderer: %s %s | %s | %s", renderer or "unknown",
		version or "unknown", vendor or "unknown", device or "unknown"))
	print("Fog/PBR sample: 25 spheres. Use Space to cycle modes; --benchmark measures 600 frames per mode.")
end

function love.update(dt)
	if not benchmark then
		return
	end
	samples = samples + 1
	total = total + dt
	minFrame = math.min(minFrame, dt)
	maxFrame = math.max(maxFrame, dt)
	if samples < 600 then
		return
	end

	local average = total * 1000 / samples
	print(string.format("%s: 600 frames, avg/min/max %.3f/%.3f/%.3f ms",
		phases[phase].name, average, minFrame * 1000, maxFrame * 1000))
	if phase < #phases then
		configure(phase + 1)
	else
		love.event.quit(0)
	end
end

function love.draw()
	love.graphics.clear(0.035, 0.045, 0.07, 1)
	g3d.begin3d()
	for _, sphere in ipairs(spheres) do
		sphere.mesh:setMaterial(sphere.material)
		g3d.draw(sphere.mesh, sphere.x, sphere.y, sphere.z)
	end
	g3d.end3d()

	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.print(phases[phase].name .. " | 25 spheres", 16, 16)
	love.graphics.print("Space: next material/fog mode | Esc: quit", 16, 40)
end

function love.resize(width, height)
	if camera and height > 0 then
		camera:setAspect(width / height)
	end
end

function love.keypressed(key)
	if key == "escape" then
		love.event.quit()
	elseif key == "space" then
		configure(phase % #phases + 1)
	end
end
