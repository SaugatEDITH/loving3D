# Creating a game with LOVING3D

LOVING3D games use the familiar LÖVE Lua lifecycle and can mix native 3D with
existing `love.graphics` 2D drawing. Use the `love` executable built from this
repository: a stock LÖVE installation does not contain `love.graphics3d`.

For build and test commands, see [Build, run, and test](development.md). For
the complete 3D API and limitations, see the [3D graphics guide](graphics3d.md).

## 1. Create a project

Create a folder such as `my-game/` containing:

```text
my-game/
├── conf.lua
├── main.lua
└── assets/
```

`conf.lua` configures the window. A 3D window needs a depth buffer:

```lua
function love.conf(t)
	t.identity = "my-game"
	t.window.title = "My LOVING3D Game"
	t.window.width = 1280
	t.window.height = 720
	t.window.resizable = true
	t.window.depth = true
end
```

## 2. Add a camera, light, and mesh

This small `main.lua` creates a lit, rotating cube and draws a 2D HUD after
the 3D pass:

```lua
local g3d = love.graphics3d
local camera
local cube
local angle = 0

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

	local material = g3d.newMaterial()
	material:setColor(0.2, 0.65, 0.95, 1)
	material:setSpecularParams(0.35, 40)

	cube = g3d.newCube()
	cube:setMaterial(material)
end

function love.update(dt)
	angle = angle + 35 * dt
end

function love.draw()
	love.graphics.clear(0.035, 0.045, 0.07, 1)

	g3d.begin3d()
	g3d.draw(cube, 0, 0, 0, 0, angle, 0, 1.5, 1.5, 1.5)
	g3d.end3d()

	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.print("My LOVING3D game", 16, 16)
end

function love.resize(width, height)
	if camera and height > 0 then
		camera:setAspect(width / height)
	end
end
```

The 3D API queues work between `begin3d()` and `end3d()`. Finish the 3D pass
before drawing 2D UI. Update the camera aspect when the viewport changes.
Create meshes/materials in `love.load`, update gameplay state in
`love.update(dt)`, and issue drawing calls in `love.draw()`.

## 3. Run and iterate

From the repository root, run the game folder with the custom engine:

```sh
./build/love ../my-game
```

You can also run a checked-in example as a starting point:

```sh
./build/love demos/graphics3d-cube
./build/love demos/graphics3d-gltf-viewer
```

For model loading, put assets inside the game folder and read them using
`love.filesystem`. For example:

```lua
local model = g3d.newModelFromFile("assets/ship.glb")
```

The current loader supports a documented subset of OBJ/MTL and glTF 2.0 /
GLB. Check the [model import guide](graphics3d.md#models-and-objmtl-import)
and [glTF limitations](graphics3d.md#gltf-20-and-glb) before choosing assets.

## 4. Test and package

Run the relevant engine tests as you build your game:

```sh
./build/love testing --modules graphics3d
./build/love testing --all
```

To package the game source as a `.love` archive, zip the **contents** of the
project so `main.lua` and `conf.lua` are at the archive root:

```sh
cd ../my-game
zip -r ../my-game.love .
cd -
./build/love ../my-game.love
```

The `.love` archive is not a standalone executable. To distribute a
standalone game, bundle it with a LOVING3D engine build for each target
platform and follow the platform's packaging steps. Review the
[license and attribution notes](licensing.md) before redistribution.
