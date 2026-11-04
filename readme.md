# LOVING3D

**LOVING3D is a LÖVE-based game engine distribution that adds native 3D rendering to LÖVE's Lua game framework.** It retains the familiar `love.*` programming model and existing 2D APIs, while adding `love.graphics3d` for 3D cameras, meshes, models, materials, lights, scene nodes, and rendering.

This repository is a modified LÖVE source tree, not an add-on package for an unmodified LÖVE installation. Build this fork and run your game with its `love` executable to use `love.graphics3d`. The 3D module is evolving; review the documented constraints and test the platforms and workloads you plan to ship.

## Get started

```sh
# Linux: build using the repository's Docker toolchain (first run builds the image).
./scripts/docker-demo.sh build

# Run a demo, or replace the path with your own game's folder or .love archive.
./build/love demos/graphics3d-cube

# Run the full suite, or just the 3D tests.
./build/love testing --all
./build/love testing --modules graphics3d
```

For native build prerequisites, focused test commands, and generated reports, see the [build and test guide](docs/development.md). To start a new project, follow [making a game with LOVING3D](docs/creating-a-game.md).

## Documentation

- **Using LÖVE's existing 2D APIs:** [Official LÖVE wiki and API reference](https://love2d.org/wiki/Main_Page). LOVING3D does not replace those APIs; 2D drawing, input, audio, filesystem, and other familiar LÖVE APIs remain part of the project.
- **Using the added 3D API:** [LOVING3D 3D graphics guide](docs/graphics3d.md).
- **Building, testing, and contributing:** [Build and test guide](docs/development.md).
- **Creating a game:** [LOVING3D game starter guide](docs/creating-a-game.md).
- **Project documentation index:** [docs/README.md](docs/README.md).
- **License and third-party notices:** [docs/licensing.md](docs/licensing.md) and [license.txt](license.txt).

## Quick start

The 3D module is exposed as `love.graphics3d`. A window using 3D rendering needs a depth buffer:

```lua
-- conf.lua
function love.conf(t)
	t.window.title = "My LOVING3D game"
	t.window.width = 1280
	t.window.height = 720
	t.window.depth = true
end
```

```lua
-- main.lua
local g3d = love.graphics3d
local camera
local cube

function love.load()
	camera = g3d.newCamera()
	camera:setPosition(4, 3, 6)
	camera:setAspect(love.graphics.getWidth() / love.graphics.getHeight())
	camera:setFOV(55)
	camera:lookAt(0, 0, 0)
	g3d.setCamera(camera)

	g3d.setAmbientColor(0.12, 0.12, 0.16)
	local sun = g3d.newLight("directional")
	sun:setDirection(-0.5, -1, -0.7)
	sun:setColor(1, 0.92, 0.78)
	sun:setIntensity(2)
	g3d.addLight(sun)

	local surface = g3d.newMaterial()
	surface:setColor(0.2, 0.65, 0.95, 1)
	surface:setSpecularParams(0.35, 40)

	cube = g3d.newCube()
	cube:setMaterial(surface)
end

function love.draw()
	love.graphics.clear(0.035, 0.045, 0.07, 1)

	g3d.begin3d()
	g3d.draw(cube, 0, 0, 0, 0, 25, 0, 1.5, 1.5, 1.5)
	g3d.end3d()

	-- Existing LÖVE 2D drawing can follow the 3D pass.
	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.print("LOVING3D", 16, 16)
end

function love.resize(width, height)
	if camera and height > 0 then
		camera:setAspect(width / height)
	end
end
```

See the [3D guide](docs/graphics3d.md) for the full API, model import, render-pass behavior, and current limitations.

## Demos

- [`demos/graphics3d-cube`](demos/graphics3d-cube): minimal 3D camera, lighting, mesh, and 2D overlay example.
- [`demos/graphics3d-canvas`](demos/graphics3d-canvas): 3D rendering to a readable Canvas, composited with 2D.
- [`demos/graphics3d-gltf-viewer`](demos/graphics3d-gltf-viewer): interactive glTF 2.0 / GLB model viewer.
- [`demos/graphics3d-resize`](demos/graphics3d-resize): resize, fullscreen, bounds, and normal visualization.
- [`demos/graphics3d-shooter`](demos/graphics3d-shooter): interactive procedural space shooter and 3D load demo, including OBJ/MTL ship import.
- [`demos/rubiks3d`](demos/rubiks3d): interactive 3D Rubik's Cube.

### Demo recordings

These looping APNG excerpts use the first half of each recording, at the original resolution and frame timing. The Rubik's Cube recording uses a reduced-color palette to keep its download size manageable.

| Rubik's Cube | Starfall |
| --- | --- |
| <img src="docs/assets/demos/rubiks-cube.apng" width="420" alt="Interactive 3D Rubik's Cube"> | <img src="docs/assets/demos/starfall.apng" width="420" alt="Starfall 3D space shooter"> |
| 3D Cube | Canvas |
| --- | --- |
| <img src="docs/assets/demos/3d-cube.apng" width="420" alt="Rotating 3D cube demo"> | <img src="docs/assets/demos/canvas.apng" width="420" alt="3D Canvas compositing demo"> |

| glTF viewer | Resize and fullscreen |
| --- | --- |
| <img src="docs/assets/demos/gltf-viewer.apng" width="420" alt="Interactive glTF model viewer"> | <img src="docs/assets/demos/resize-fullscreen.apng" width="420" alt="3D resize and fullscreen demo"> |


## Building

See the [build and test guide](docs/development.md) for native CMake builds, the Docker build environment, running demos, and tests.

## 3D implementation overview

- Right-handed world coordinates, +Y up, camera forward along -Z by default.
- Perspective and orthographic cameras.
- Indexed meshes, procedural primitives, OBJ/MTL, and partial glTF 2.0 / GLB loading.
- Blinn-Phong and direct-light metallic-roughness materials; directional, point, and spot lights.
- Depth-tested opaque/transparent passes, fog, Canvas rendering, a transform scene graph, GPU instancing, automatic opaque batching, and distance-based mesh LOD.
- Existing `love.graphics` APIs remain available for 2D drawing and UI.

LOVING3D is an evolving 3D foundation, not a claim of feature parity with mature 3D engines. Skeletal animation, 3D physics, shadow mapping, image-based lighting, and an extensible render graph are not currently provided. Check the [feature status and limitations](docs/graphics3d.md#feature-status) and validate the exact backend and hardware you plan to ship on.

## Contributing and support

Please open an issue with a minimal reproduction, platform, build configuration, and relevant logs when reporting a problem. Keep changes scoped, preserve upstream LÖVE notices, and include tests or a reproducible runtime check for behavior changes.

## Licensing

LOVING3D modifies and redistributes LÖVE source code. Lua's license permits broad use of Lua, but it does not replace the licenses or copyright notices for LÖVE and the other components in this source tree. This repository contains multiple third-party components and notices; review [license.txt](license.txt), source-file notices, and [docs/licensing.md](docs/licensing.md) when redistributing or incorporating the project.

LOVING3D is an independent project and is not an official LÖVE release. LÖVE and Lua retain their respective names, copyrights, and licensing terms.
