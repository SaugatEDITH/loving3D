--
-- LOVING3D - Starfall, an interactive native 3D space shooter.
--
-- Copyright (c) 2026 Saugat Pokharel
--
-- This file is part of LOVING3D, an independent extension adding native 3D
-- capabilities to the LOVE game engine (love2d.org). The underlying engine
-- remains freely distributable under the terms of its original zlib license
-- (see license.txt).
--

local g3d = love.graphics3d
local random = love.math.random
local abs, min, max, sqrt = math.abs, math.min, math.max, math.sqrt
local sin, cos, pi = math.sin, math.cos, math.pi

local WIDTH, HEIGHT = 24, 16
local PLAYER_Z = 0
local LEVELS = {
	{ name = "EASY", stars = 70, asteroids = 8, enemies = 4, bullets = 30 },
	{ name = "ARCADE", stars = 150, asteroids = 12, enemies = 7, bullets = 55 },
	{ name = "STRESS", stars = 420, asteroids = 24, enemies = 14, bullets = 100 },
}

local camera
local ship
local shipMaterials
local meshes = {}
local materials = {}
local stars = {}
local asteroids = {}
local enemies = {}
local bullets = {}
local particles = {}
local quality = 2
local score = 0
local lives = 3
local gameOver = false
local paused = false
local showHelp = true
local playerX, playerY = 0, -2
local fireClock = 0
local spawnClock = 0
local invulnerable = 0
local elapsed = 0
local frameCounter = 0

local function makeDiffuseTexture()
	local data = love.image.newImageData(32, 32)
	for y = 0, 31 do
		for x = 0, 31 do
			local stripe = ((x + y) % 8) < 2
			if stripe then
				data:setPixel(x, y, 0.15, 0.78, 0.95, 1)
			else
				data:setPixel(x, y, 0.52, 0.72, 0.84, 1)
			end
		end
	end
	local image = love.graphics.newImage(data)
	image:setFilter("linear", "linear")
	return image
end

local function readAsset(path)
	local data, message = love.filesystem.read(path)
	assert(data, "Unable to read " .. path .. ": " .. tostring(message))
	return data
end

local function newEntity(x, y, z, scale, speed)
	return {
		x = x, y = y, z = z,
		scale = scale,
		speed = speed,
		rotation = random() * 360,
		spin = random(-80, 80),
		phase = random() * pi * 2,
	}
end

local function buildScene()
	camera = g3d.newCamera()
	camera:setPosition(0, 2.3, 18)
	camera:setAspect(love.graphics.getWidth() / love.graphics.getHeight())
	camera:setFOV(55)
	camera:setNear(0.1)
	camera:setFar(140)
	camera:lookAt(0, 0, -7)
	g3d.setCamera(camera)
	g3d.setAutoBatching(false)

	g3d.setAmbientColor(0.16, 0.19, 0.28)

	local sun = g3d.newLight("directional")
	sun:setDirection(-0.45, -0.7, -1)
	sun:setColor(0.72, 0.84, 1)
	sun:setIntensity(1.7)
	g3d.addLight(sun)

	local fill = g3d.newLight("point")
	fill:setPosition(3, 4, 8)
	fill:setColor(0.35, 0.45, 1)
	fill:setIntensity(11)
	fill:setRange(35)
	fill:setFalloff(0.7)
	g3d.addLight(fill)

	materials.star = g3d.newMaterial()
	materials.star:setColor(0.72, 0.84, 1, 1)
	materials.star:setUnlit(true)

	materials.rocks = {}
	local rockPalettes = {
		{ color = { 0.72, 0.43, 0.22 }, glow = { 0.11, 0.045, 0.012 } },
		{ color = { 0.34, 0.64, 0.76 }, glow = { 0.025, 0.085, 0.13 } },
		{ color = { 0.68, 0.72, 0.78 }, glow = { 0.065, 0.075, 0.09 } },
	}
	for i, palette in ipairs(rockPalettes) do
		local material = g3d.newMaterial()
		material:setColor(palette.color[1], palette.color[2], palette.color[3], 1)
		material:setEmissiveColor(palette.glow[1], palette.glow[2], palette.glow[3])
		material:setSpecularParams(0.34, 20)
		materials.rocks[i] = material
	end

	materials.enemy = g3d.newMaterial()
	materials.enemy:setColor(0.92, 0.19, 0.16, 1)
	materials.enemy:setSpecularParams(0.3, 24)

	materials.bullet = g3d.newMaterial()
	materials.bullet:setColor(0.3, 0.85, 1, 1)
	materials.bullet:setUnlit(true)

	materials.particle = g3d.newMaterial()
	materials.particle:setColor(1, 0.54, 0.15, 1)
	materials.particle:setUnlit(true)

	meshes.star = g3d.newSphere(6, 8, 1)
	meshes.star:setMaterial(materials.star)
	meshes.rocks = {}
	for i, material in ipairs(materials.rocks) do
		meshes.rocks[i] = g3d.newSphere(8, 10, 1)
		meshes.rocks[i]:setMaterial(material)
	end
	meshes.enemy = g3d.newCone(8, 0.7, 1.7)
	meshes.enemy:setMaterial(materials.enemy)
	meshes.bullet = g3d.newSphere(6, 8, 1)
	meshes.bullet:setMaterial(materials.bullet)
	meshes.particle = g3d.newCube()
	meshes.particle:setMaterial(materials.particle)

	local texture = makeDiffuseTexture()
	local obj = readAsset("assets/pilot.obj")
	local mtl = readAsset("assets/pilot.mtl")
	ship, shipMaterials = g3d.newModel(obj, mtl, { ["pilot_diffuse.png"] = texture })
	assert(ship and shipMaterials and #shipMaterials >= 3, "OBJ/MTL ship did not import its three materials")

	local textureBound = false
	for _, material in ipairs(shipMaterials) do
		material:setSpecularParams(0.42, 42)
		if material:getDiffuseTexture() then
			textureBound = true
		end
	end
	assert(textureBound, "OBJ/MTL diffuse texture was not bound to the imported ship")
end

local function populateBackground()
	local level = LEVELS[quality]
	stars, asteroids = {}, {}

	for i = 1, level.stars do
		stars[i] = newEntity(random(-34, 34), random(-23, 23), random(-76, -40),
			random(7, 12) / 100, random(3, 11))
	end

	for i = 1, level.asteroids do
		local rock = newEntity(random(-11, 11), random(-7, 7), random(-55, -14),
			random(45, 90) / 100, random(7, 14))
		rock.variant = random(1, #meshes.rocks)
		asteroids[i] = rock
	end
end

local function resetGame()
	score = 0
	lives = 3
	gameOver = false
	paused = false
	playerX, playerY = 0, -2
	fireClock, spawnClock, invulnerable, elapsed = 0, 0, 0, 0
	enemies, bullets, particles = {}, {}, {}
	populateBackground()
end

local function draw(mesh, x, y, z, rx, ry, rz, sx, sy, sz)
	g3d.draw(mesh, x, y, z, rx or 0, ry or 0, rz or 0, sx or 1, sy or 1, sz or 1)
end

local function spawnEnemy()
	local level = LEVELS[quality]
	if #enemies >= level.enemies then
		return
	end

	local side = random(1, 2) == 1 and -1 or 1
	enemies[#enemies + 1] = {
		x = random(-9, 9),
		y = random(-5, 6),
		z = -48,
		speed = random(45, 80) / 10 + min(score / 120, 4),
		wobble = random(15, 35) / 10,
		phase = random() * pi * 2,
		side = side,
		scale = random(8, 13) / 10,
	}
end

local function fireBullet()
	if #bullets >= LEVELS[quality].bullets then
		return
	end

	bullets[#bullets + 1] = {
		x = playerX, y = playerY + 0.18, z = PLAYER_Z - 1.1,
		speed = 42, life = 1.8,
	}
end

local function burst(x, y, z, count, color)
	for i = 1, count do
		local angle = random() * pi * 2
		local speed = random(25, 100) / 10
		particles[#particles + 1] = {
			x = x, y = y, z = z,
			vx = cos(angle) * speed,
			vy = sin(angle) * speed,
			vz = random(-35, 35) / 10,
			life = random(25, 60) / 100,
			color = color,
			scale = random(4, 12) / 100,
		}
	end
end

local function hitPlayer()
	if invulnerable > 0 or gameOver then
		return
	end

	lives = lives - 1
	invulnerable = 1.7
	burst(playerX, playerY, PLAYER_Z, 24)
	if lives <= 0 then
		gameOver = true
	end
end

local function updateStars(dt)
	for _, star in ipairs(stars) do
		star.z = star.z + star.speed * dt
		if star.z > -38 then
			star.z = random(-82, -68)
			star.x = random(-34, 34)
			star.y = random(-23, 23)
		end
	end
end

local function updateAsteroids(dt)
	for _, rock in ipairs(asteroids) do
		rock.z = rock.z + rock.speed * dt
		rock.rotation = rock.rotation + rock.spin * dt
		rock.phase = rock.phase + dt * 0.35
		rock.x = rock.x + sin(rock.phase) * dt * 0.18

		if rock.z > 0 then
			rock.z = random(-55, -36)
			rock.x = random(-11, 11)
			rock.y = random(-7, 7)
			rock.scale = random(45, 90) / 100
			rock.variant = random(1, #meshes.rocks)
		end

		if abs(rock.z - PLAYER_Z) < rock.scale + 0.8
			and (rock.x - playerX) ^ 2 + (rock.y - playerY) ^ 2 < (rock.scale + 0.65) ^ 2 then
			rock.z = -80
			hitPlayer()
		end
	end
end

local function updateCombat(dt)
	local speed = love.keyboard.isDown("lshift", "rshift") and 10 or 7
	local dx, dy = 0, 0
	if love.keyboard.isDown("a", "left") then dx = dx - 1 end
	if love.keyboard.isDown("d", "right") then dx = dx + 1 end
	if love.keyboard.isDown("w", "up") then dy = dy + 1 end
	if love.keyboard.isDown("s", "down") then dy = dy - 1 end

	local magnitude = sqrt(dx * dx + dy * dy)
	if magnitude > 0 then
		playerX = playerX + dx / magnitude * speed * dt
		playerY = playerY + dy / magnitude * speed * dt
	end
	playerX = max(-10, min(10, playerX))
	playerY = max(-6.3, min(6.3, playerY))

	if invulnerable > 0 then invulnerable = max(0, invulnerable - dt) end
	fireClock = max(0, fireClock - dt)
	if (love.keyboard.isDown("space") or love.mouse.isDown(1)) and fireClock == 0 then
		fireBullet()
		fireClock = 0.13
	end

	spawnClock = spawnClock - dt
	if spawnClock <= 0 then
		spawnEnemy()
		spawnClock = max(0.3, 1.05 - min(score / 800, 0.55)) * random(75, 125) / 100
	end

	for i = #bullets, 1, -1 do
		local bullet = bullets[i]
		bullet.z = bullet.z - bullet.speed * dt
		bullet.life = bullet.life - dt
		if bullet.life <= 0 or bullet.z < -58 then
			table.remove(bullets, i)
		end
	end

	for i = #enemies, 1, -1 do
		local enemy = enemies[i]
		enemy.z = enemy.z + enemy.speed * dt
		enemy.phase = enemy.phase + dt * enemy.wobble
		enemy.x = enemy.x + sin(enemy.phase) * dt * enemy.side
		enemy.y = enemy.y + cos(enemy.phase * 0.7) * dt * 0.25

		if enemy.z > 3.2 then
			table.remove(enemies, i)
			hitPlayer()
		elseif abs(enemy.z - PLAYER_Z) < 1.2
			and (enemy.x - playerX) ^ 2 + (enemy.y - playerY) ^ 2 < 1.45 ^ 2 then
			table.remove(enemies, i)
			burst(enemy.x, enemy.y, enemy.z, 20)
			hitPlayer()
		else
			for j = #bullets, 1, -1 do
				local bullet = bullets[j]
				if abs(enemy.z - bullet.z) < 0.85
					and (enemy.x - bullet.x) ^ 2 + (enemy.y - bullet.y) ^ 2 < 0.95 ^ 2 then
					table.remove(enemies, i)
					table.remove(bullets, j)
					score = score + 100
					burst(enemy.x, enemy.y, enemy.z, 15)
					break
				end
			end
		end
	end

	for i = #particles, 1, -1 do
		local particle = particles[i]
		particle.life = particle.life - dt
		if particle.life <= 0 then
			table.remove(particles, i)
		else
			particle.x = particle.x + particle.vx * dt
			particle.y = particle.y + particle.vy * dt
			particle.z = particle.z + particle.vz * dt
		end
	end
end

local function drawWorld()
	for _, star in ipairs(stars) do
		draw(meshes.star, star.x, star.y, star.z, 0, 0, 0,
			star.scale, star.scale, star.scale)
	end

	for _, rock in ipairs(asteroids) do
		draw(meshes.rocks[rock.variant], rock.x, rock.y, rock.z,
			rock.rotation, rock.rotation * 0.63, rock.rotation * 0.31,
			rock.scale, rock.scale * 0.78, rock.scale * 1.12)
	end

	for _, enemy in ipairs(enemies) do
		draw(meshes.enemy, enemy.x, enemy.y, enemy.z, 90, enemy.phase * 15, 0,
			enemy.scale, enemy.scale, enemy.scale)
	end

	for _, bullet in ipairs(bullets) do
		draw(meshes.bullet, bullet.x, bullet.y, bullet.z, 0, 0, 0, 0.09, 0.09, 0.42)
	end

	for _, particle in ipairs(particles) do
		draw(meshes.particle, particle.x, particle.y, particle.z,
			0, elapsed * 70, 0, particle.scale, particle.scale, particle.scale)
	end

	local flash = invulnerable > 0 and math.floor(invulnerable * 12) % 2 == 0
	if not flash then
		g3d.draw(ship, playerX, playerY, PLAYER_Z, 0, sin(elapsed * 4) * 3, 0, 0.78, 0.78, 0.78)
	end
end

local function getNearestThreat()
	local nearest, nearestDepth
	for _, rock in ipairs(asteroids) do
		local dx, dy = rock.x - playerX, rock.y - playerY
		if rock.z > -20 and rock.z < 1 and dx * dx + dy * dy < (rock.scale + 2.8) ^ 2
			and (nearestDepth == nil or rock.z > nearestDepth) then
			nearest, nearestDepth = rock, rock.z
		end
	end

	for _, enemy in ipairs(enemies) do
		local dx, dy = enemy.x - playerX, enemy.y - playerY
		if enemy.z > -20 and enemy.z < 1 and dx * dx + dy * dy < 3.8 ^ 2
			and (nearestDepth == nil or enemy.z > nearestDepth) then
			nearest, nearestDepth = enemy, enemy.z
		end
	end
	return nearest
end

local function drawRadar()
	local screenW, screenH = love.graphics.getDimensions()
	local panelW, panelH = 184, 158
	local left, top = screenW - panelW - 14, screenH - panelH - 14
	local innerLeft, innerTop = left + 18, top + 32
	local innerW, innerH = panelW - 36, panelH - 57

	love.graphics.setColor(0.015, 0.035, 0.075, 0.9)
	love.graphics.rectangle("fill", left, top, panelW, panelH, 5, 5)
	love.graphics.setColor(0.16, 0.42, 0.58, 0.9)
	love.graphics.rectangle("line", left, top, panelW, panelH, 5, 5)
	love.graphics.setColor(0.53, 0.83, 1, 1)
	love.graphics.print("RANGE  /  LEFT - RIGHT", left + 10, top + 9)

	love.graphics.setColor(0.17, 0.34, 0.43, 0.75)
	for i = 0, 4 do
		local x = innerLeft + innerW * i / 4
		love.graphics.line(x, innerTop, x, innerTop + innerH)
	end
	for i = 0, 3 do
		local y = innerTop + innerH * i / 3
		love.graphics.line(innerLeft, y, innerLeft + innerW, y)
	end

	local function radarPoint(x, z)
		return innerLeft + (x + 12) / 24 * innerW,
			innerTop + (z + 58) / 58 * innerH
	end

	for _, rock in ipairs(asteroids) do
		if rock.z >= -58 and rock.z < 0 and abs(rock.x) <= 12 then
			local x, y = radarPoint(rock.x, rock.z)
			local color = rock.variant == 1 and { 1, 0.62, 0.3 }
				or rock.variant == 2 and { 0.38, 0.82, 1 }
				or { 0.86, 0.91, 1 }
			love.graphics.setColor(color[1], color[2], color[3], 1)
			love.graphics.circle("fill", x, y, max(2.5, min(5, rock.scale * 2.5)))
		end
	end

	for _, enemy in ipairs(enemies) do
		if enemy.z >= -58 and enemy.z < 0 and abs(enemy.x) <= 12 then
			local x, y = radarPoint(enemy.x, enemy.z)
			love.graphics.setColor(1, 0.24, 0.2, 1)
			love.graphics.polygon("fill", x, y - 4, x - 3.5, y + 3, x + 3.5, y + 3)
		end
	end

	local playerRadarX = innerLeft + (playerX + 12) / 24 * innerW
	love.graphics.setColor(0.45, 1, 0.68, 1)
	love.graphics.polygon("fill", playerRadarX, innerTop + innerH + 5,
		playerRadarX - 5, innerTop + innerH - 2, playerRadarX + 5, innerTop + innerH - 2)
	love.graphics.setColor(0.69, 0.81, 0.9, 1)
	love.graphics.print("FAR", left + 7, innerTop + 1)
	love.graphics.print("NEAR", left + 2, innerTop + innerH - 9)
end

local function drawHUD()
	local level = LEVELS[quality]
	local stats = g3d.getStats()
	local fps = love.timer.getFPS()

	love.graphics.setColor(0.02, 0.04, 0.09, 0.78)
	love.graphics.rectangle("fill", 12, 12, 260, 82)
	love.graphics.setColor(0.48, 0.85, 1, 1)
	love.graphics.print("STARFALL  /  3D STRESS TEST", 24, 22)
	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.print(string.format("SCORE %06d    HULL %d", score, lives), 24, 44)
	love.graphics.print(string.format("%s  |  FPS %d  |  3D DRAWS %d", level.name, fps, stats.drawsPassed), 24, 66)

	local threat = getNearestThreat()
	if threat ~= nil then
		local dx = threat.x - playerX
		local direction = dx < -0.8 and "LEFT" or dx > 0.8 and "RIGHT" or "CENTER"
		local distance = max(0, -threat.z)
		local bannerW, bannerH = 310, 34
		local bannerX = (love.graphics.getWidth() - bannerW) * 0.5
		love.graphics.setColor(0.32, 0.055, 0.025, 0.92)
		love.graphics.rectangle("fill", bannerX, 15, bannerW, bannerH, 4, 4)
		love.graphics.setColor(1, 0.72, 0.28, 1)
		love.graphics.printf(string.format("WARNING  %s  /  RANGE %02d", direction, distance),
			bannerX, 24, bannerW, "center")
	end

	if showHelp then
		love.graphics.setColor(0.02, 0.04, 0.09, 0.78)
		love.graphics.rectangle("fill", 12, love.graphics.getHeight() - 112, 440, 100)
		love.graphics.setColor(0.82, 0.9, 1, 1)
		love.graphics.print("WASD / arrows move   SPACE / mouse fire   SHIFT boost", 24, love.graphics.getHeight() - 99)
		love.graphics.print("P pause   R restart   H help   1/2/3 load level   ESC quit", 24, love.graphics.getHeight() - 77)
		love.graphics.print("OBJ + MTL ship, generated texture, procedural mesh fleet", 24, love.graphics.getHeight() - 55)
	end

	drawRadar()

	if paused or gameOver then
		love.graphics.setColor(0.01, 0.02, 0.05, 0.82)
		love.graphics.rectangle("fill", 0, 0, love.graphics.getWidth(), love.graphics.getHeight())
		love.graphics.setColor(gameOver and 1 or 0.45, gameOver and 0.35 or 0.9, 1, 1)
		local message = gameOver and "SHIP LOST" or "PAUSED"
		love.graphics.printf(message .. "   -   " .. (gameOver and "PRESS R TO FLY AGAIN" or "PRESS P TO RESUME"),
			0, love.graphics.getHeight() * 0.46, love.graphics.getWidth(), "center")
	end
end

function love.load()
	love.math.setRandomSeed(os.time())
	buildScene()
	populateBackground()
	print("Starfall ready: OBJ/MTL import, generated texture, procedural 3D combat")
end

function love.update(dt)
	dt = min(dt, 0.05)
	if paused or gameOver then return end

	elapsed = elapsed + dt
	frameCounter = frameCounter + 1
	updateStars(dt)
	updateAsteroids(dt)
	updateCombat(dt)

	if frameCounter % 600 == 0 then
		local stats = g3d.getStats()
		print(string.format("Starfall: %s, %d 3D draws, %d enemies, score %d",
			LEVELS[quality].name, stats.drawsPassed, #enemies, score))
	end
end

function love.draw()
	love.graphics.clear(0.008, 0.014, 0.04, 1)
	g3d.begin3d()
	drawWorld()
	g3d.end3d()
	drawHUD()
end

function love.keypressed(key)
	if key == "escape" then
		love.event.quit()
	elseif key == "p" and not gameOver then
		paused = not paused
	elseif key == "h" then
		showHelp = not showHelp
	elseif key == "r" then
		resetGame()
	elseif key == "1" or key == "2" or key == "3" then
		quality = tonumber(key)
		resetGame()
	end
end

function love.resize(width, height)
	if camera ~= nil and height > 0 then
		camera:setAspect(width / height)
	end
end
