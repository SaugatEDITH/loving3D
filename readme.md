# LOVING3D

**LOVING3D is a LÖVE-based game engine distribution that adds native 3D rendering to LÖVE's Lua game framework.** It retains the familiar `love.*` programming model and existing 2D APIs, while adding the `love.graphics3d` module for 3D cameras, meshes, models, materials, lights, and rendering.

This repository is a modified LÖVE source tree, not an add-on package that can be dropped into an unmodified LÖVE installation. The 3D module is an evolving implementation; review the documented constraints and test the exact platforms and workloads you plan to ship.

## Documentation

- **Using LÖVE's existing 2D APIs:** [Official LÖVE wiki and API reference](https://love2d.org/wiki/Main_Page). LOVING3D does not replace those APIs; 2D drawing, input, audio, filesystem, and other familiar LÖVE APIs remain part of the project.
- **Using the added 3D API:** [LOVING3D 3D graphics guide](docs/graphics3d.md).
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
- [`demos/graphics3d-shooter`](demos/graphics3d-shooter): interactive procedural space shooter and 3D load demo, including OBJ/MTL ship import.
- [`demos/rubiks3d`](demos/rubiks3d): 3D shapes and rendering example.

Run a demo with the built executable, for example:

```sh
./build/love demos/graphics3d-shooter
```

## Building

Build requirements depend on the target platform and the dependencies configured for this LÖVE source tree. With the required CMake toolchain and development libraries installed:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j"$(nproc)"
```

On Linux, this repository also includes a Docker build environment:

```sh
docker build -t loving3d-build -f docker/Dockerfile .
./scripts/docker-demo.sh build
```

The Docker script bind-mounts the working tree and writes build output to `build/`. Display and GPU availability inside containers depends on the host setup. The included container configuration is a build/test convenience, not a platform support guarantee.

## 3D implementation overview

- Right-handed world coordinates, +Y up, camera forward along -Z by default.
- Perspective and orthographic cameras.
- Indexed meshes, built-in procedural primitives, and a Wavefront OBJ/MTL text importer.
- Per-mesh/model materials with diffuse, normal, specular, and emissive texture slots.
- Blinn-Phong lighting with directional, point, and spot lights (up to eight active lights).
- Depth-tested rendering, opaque-first submission, and back-to-front sorting for transparent draws.
- `love.graphics` remains available for 2D rendering and UI.

The current renderer is a useful foundation, not a claim of feature parity with mature 3D engines. It does not currently document a full scene graph, skeletal animation, a physics integration, glTF import, shadow mapping, or a render graph. Consult [known constraints](docs/graphics3d.md#current-scope-and-limitations) before choosing it for a production project.

## Contributing and support

Please open an issue with a minimal reproduction, platform, build configuration, and relevant logs when reporting a problem. Keep changes scoped, preserve upstream LÖVE notices, and include tests or a reproducible runtime check for behavior changes.

## Licensing

LOVING3D modifies and redistributes LÖVE source code. Lua's license permits broad use of Lua, but it does not replace the licenses or copyright notices for LÖVE and the other components in this source tree. This repository contains multiple third-party components and notices; review [license.txt](license.txt), source-file notices, and [docs/licensing.md](docs/licensing.md) when redistributing or incorporating the project.

LOVING3D is an independent project and is not an official LÖVE release. LÖVE and Lua retain their respective names, copyrights, and licensing terms.
