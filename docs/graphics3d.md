# `love.graphics3d`

`love.graphics3d` is LOVING3D's native 3D rendering module. It adds 3D resources and a queued 3D pass to the existing LÖVE application lifecycle; it does not replace `love.graphics` or the rest of the LÖVE API.

## Requirements and render lifecycle

Enable a depth buffer for the main window in `conf.lua`:

```lua
function love.conf(t)
	t.window.depth = true
end
```

Create resources and configure the camera/lights outside the draw pass. In `love.draw`, begin the 3D pass, queue 3D draws, then end the pass before issuing normal 2D drawing:

```lua
function love.draw()
	love.graphics.clear(0.02, 0.03, 0.06, 1)

	love.graphics3d.begin3d()
	love.graphics3d.draw(mesh, 0, 0, 0)
	love.graphics3d.end3d()

	love.graphics.setColor(1, 1, 1, 1)
	love.graphics.print("2D overlay", 12, 12)
end
```

`draw` calls made between `begin3d` and `end3d` are queued and submitted when `end3d` is called. Opaque draws are submitted before transparent draws; transparent draws are sorted back-to-front and rendered without depth writes. Always pair the calls in the same frame and do not assume immediate-mode rendering between them. The 3D pass manages and restores the graphics state it changes, but applications should still keep their own 2D drawing outside the pass.

The main window must have a depth buffer. Canvas rendering may attach a temporary depth buffer when required by the implementation. Use `clearDepth()` only when you intentionally need to reset depth testing for another layer of 3D draws; it clears the active target's depth buffer, not its color.

## Feature status

| Area | Status | Notes |
|---|---|---|
| Camera projection, depth testing, face culling, opaque/transparent passes | Implemented | Transparent sorting is approximate and per draw. |
| OBJ/MTL, Blinn-Phong materials, directional/point/spot lights | Implemented | OBJ/MTL is a focused subset. |
| Camera queries, mesh/model bounds, frustum culling, basic draw statistics | Implemented | Culling uses transformed AABBs; statistics are not a full profiler. |
| Debug lines, bounds, spheres, and mesh normals | Implemented | Drawn through the active 3D pass. |
| Canvas rendering and 2D compositing | Implemented | Includes a temporary depth attachment when needed. |
| Parent/child scene graph | Implemented | `Node3D` composes local TRS transforms and recursively queues attached meshes/models. |
| GPU instancing and repeated opaque draw batching | Implemented | Requires backend instancing support; transparent meshes remain individually sorted. |
| Distance-based mesh LOD | Implemented | Selects a registered replacement mesh using camera distance to the object's bounds center. |
| Material fragment hook and shader reload | Implemented | The hook wraps the standard Blinn-Phong shader; full arbitrary render pipelines are not exposed. |
| Linear and exponential fog | Implemented | Configured through `setFog`; fog is applied to lit and unlit materials. |
| Cubemap and procedural skybox | Partial | Not implemented yet; no skybox API or background pass is present. |
| Directional shadow maps, PCF, and bias controls | Partial | Not implemented yet; no shadow map render or sample pass is present. |
| Metallic-roughness PBR, occlusion mapping, image-based lighting | Partial | Direct-light GGX metallic-roughness shading and glTF map bindings are implemented; image-based lighting is not. |
| Post-processing, HDR, bloom, SSAO, temporal AA | Partial | Not implemented yet; no post-processing graph or HDR pipeline is present. |
| glTF 2.0 / GLB static meshes and core material textures | Partial | `newModelFromFile` imports triangle meshes, node transforms, buffers, base color/normal/emissive textures, alpha modes, and samplers. Sparse accessors and extensions are rejected; PBR, animation, and non-triangle modes are not implemented. |
| Skeletal animation, animation blending/state machines | Partial | Not implemented yet; skinning, clips, blending, and animation state machines are absent. |
| ECS, 3D physics/controllers, scene serialization | Partial | A transform scene graph is provided, but there is no ECS, physics simulation, character controller, or scene format. |
| Async asset streaming | Partial | Not implemented yet; asset loading is synchronous. |

## Coordinate system and units

- Right-handed coordinates.
- +X is right, +Y is up, and the default camera forward direction is -Z.
- Use a consistent world-unit scale across camera clipping planes, geometry, and light ranges.
- Camera field of view, camera/object rotations, and spot-light angles are in degrees.
- Draw scales are independent on X, Y, and Z.
- Camera aspect ratio is width divided by height; update it in `love.resize`.

The renderer does not impose a physical unit such as meters. Applications define and consistently use their own scale.

## Minimal setup

```lua
local g3d = love.graphics3d
local camera
local box

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

	box = g3d.newBox(1, 1, 1)
	box:setMaterial(material)
end

function love.draw()
	g3d.begin3d()
	g3d.draw(box, 0, 0, 0)
	g3d.end3d()
end

function love.resize(width, height)
	if camera and height > 0 then
		camera:setAspect(width / height)
	end
end
```

## Module API

### Scene and rendering functions

| Function | Description |
|---|---|
| `newCamera()` | Create a camera. |
| `newMaterial()` | Create a material. |
| `newNode([meshOrModel])` | Create a transform node, optionally with a `Mesh3D` or `Model3D` attached. |
| `newModelFromFile(path)` | Load an OBJ, glTF 2.0, or GLB file and resolve relative buffers, material libraries, and images through `love.filesystem`. |
| `newLight([type])` | Create a `directional`, `point`, or `spot` light; defaults to `point`. |
| `setCamera(camera)` / `getCamera()` | Set or retrieve the active scene camera. |
| `addLight(light)` / `removeLight(light)` / `clearLights()` | Manage active scene lights. |
| `setAmbientColor(r, g, b)` / `getAmbientColor()` | Set/retrieve scene ambient color. |
| `setFog(mode, r, g, b, density, start, end)` | Configure `none`, `linear`, or `exp` fog; color defaults to `(0.5, 0.6, 0.7)`, density to `0.01`, and linear distances to `10` and `100`. |
| `begin3d()` / `end3d()` | Begin the queued 3D pass and submit it. |
| `draw(meshOrModel, x, y, z, [rx, ry, rz], [sx, sy, sz])` | Queue a mesh or model with optional Euler rotation and scale. |
| `draw(node)` | Queue a node's attached mesh/model and all descendants using world transforms. |
| `drawInstanced(mesh, transforms)` | Queue multiple transforms for one mesh. `transforms` is an array of 9-value TRS records, 16-value column-major matrices, or `ByteData` containing packed float32 matrices (64 bytes each). |
| `setAutoBatching(enabled)` / `isAutoBatchingEnabled()` | Enable automatic grouping of repeated opaque mesh/material draws; disable it for diagnostic comparisons. Explicit `drawInstanced` still batches when automatic batching is off. |
| `clearDepth()` | Clear depth on the active render target. |
| `setWinding(winding)` / `getWinding()` | Set/retrieve front-face winding (`ccw` or `cw`). |
| `debugLine(x1,y1,z1, x2,y2,z2, [r,g,b,a], [depthTest])` | Queue a colored line in the 3D pass. |
| `debugBox(minX,minY,minZ, maxX,maxY,maxZ, [r,g,b,a], [depthTest])` | Queue a world-space AABB outline. |
| `debugBox(meshOrModel, x,y,z, rx,ry,rz, sx,sy,sz, [depthTest])` | Queue the transformed local bounds outline. |
| `debugSphere(x,y,z, radius, [r,g,b,a], [depthTest])` | Queue a wireframe sphere. |
| `debugNormals(mesh, x,y,z, rx,ry,rz, sx,sy,sz, [length], [depthTest])` | Queue sampled vertex normals; at most 2048 per call. |
| `getStats()` | Return `drawsPassed`, `drawsFlushed`, `drawCalls`, `triangles`, `culled`, `lightsUsed`, `queued`, and `maxLights`. |

Angles are degrees. Rotation and scale arguments are optional as groups: provide all three rotation values before optionally providing all three scale values. Defaults are zero rotation and unit scale. Euler rotations are **intrinsic**, applied in Z, then X, then Y order (the composed quaternion is `qY * qX * qZ`). Intrinsic rotations rotate about the object's changing local axes; this differs from applying extrinsic rotations about fixed world axes.

`drawsPassed` counts queued mesh draws in the current/most recent pass (a model can queue multiple draws) and resets at `begin3d`. `drawCalls`, `triangles`, `culled`, `lightsUsed`, and `queued` describe the most recent pass and reset at the next `begin3d`. `drawsFlushed` is cumulative submitted GPU draws and is not a per-frame profiler counter. `maxLights` is the module limit.

Automatic batching is for opaque draws with the same mesh and material. On backends supporting instancing, matching draws are submitted as one instanced command; otherwise they fall back to individual commands. Transparent draws keep their back-to-front order. Instancing transforms are per-instance world matrices; each object is still independently frustum-culled. `setAutoBatching(false)` is a diagnostic switch for measuring an unbatched baseline.

The transparent sort key is the camera-space depth of each draw's world-space AABB center, sorted far-to-near. It is not a per-triangle sort and does not correctly order intersecting transparent meshes.

### Cameras

`newCamera()` creates a perspective camera with defaults of 70-degree vertical FOV, 16:9 aspect, 0.1 near plane, and 1000 far plane. Set the aspect to match the active viewport.

| Method | Description |
|---|---|
| `setPosition(x, y, z)` / `getPosition()` | World-space camera position; getter returns three numbers. |
| `setRotation(rx, ry, rz)` / `getRotation()` | Set/get Euler rotation in degrees. |
| `lookAt(x, y, z)` | Aim the camera at a world-space point. |
| `setFOV(degrees)` / `getFOV()` | Perspective vertical field of view. |
| `setNear(distance)` / `getNear()` | Near clipping distance; must be positive. |
| `setFar(distance)` / `getFar()` | Far clipping distance; must be greater than near. |
| `setAspect(widthOverHeight)` / `getAspect()` | Perspective aspect ratio; must be positive. |
| `setProjectionMode(mode)` / `getProjectionMode()` | Select `perspective` or `orthographic`. |
| `setOrthoExtents(left, right, bottom, top)` / `getOrthoExtents()` | Set/get orthographic bounds in world units. |
| `worldToScreen(x,y,z, [viewportW,viewportH])` | Return top-left-origin viewport pixel coordinates and positive-forward camera-space depth. |
| `screenToWorld(sx,sy,depth, [viewportW,viewportH])` | Inverse query using the same forward depth returned by `worldToScreen`. |
| `screenRay(sx,sy, [viewportW,viewportH])` | Return ray origin and direction as six numbers. Perspective rays start at the camera; orthographic rays start at the corresponding camera-plane position. |
| `getForward()` / `getRight()` / `getUp()` | Return the camera's normalized world-space basis vectors. |
| `moveRelative(forward, strafe, up)` | Move in camera-relative directions. |
| `rotateYaw(degrees)` / `rotatePitch(degrees)` / `rotateRoll(degrees)` | Rotate the camera. |

`lookAt` establishes an orientation toward the target. For a controllable camera, update it through the camera methods and keep the aspect ratio current when the window or render target changes size. The optional query viewport dimensions are in pixels and default to the window dimensions; pass the Canvas dimensions when querying a Canvas view, and set the camera aspect to that Canvas's width divided by height so queries match its rendered projection.

### Meshes and procedural primitives

`newMesh(vertices, [indices], [usage])` accepts an array of vertex arrays. Each vertex is:

```lua
{ px, py, pz, nx, ny, nz, u, v, r, g, b, a }
```

Position is required. Omitted trailing values keep defaults: normal `(0, 1, 0)`, UV `(0, 0)`, and vertex color `(1, 1, 1, 1)`. Indices are 1-based Lua indices into the vertex array. If indices are omitted, vertices are interpreted sequentially as triangles; provide a vertex count divisible by three. Invalid indices, non-finite vertex data, or invalid geometry raise an error. The optional usage string is a LOVE buffer-usage constant and defaults to `static`.

Meshes expose `getVertexCount()`, `getIndexCount()`, `getBounds()` (six local-space AABB numbers, or `nil` for empty geometry), `setMaterial(material)`, and `getMaterial()`. Models expose `getPartCount()` and the same six-number local-space `getBounds()` result, unioning their part bounds.

| Constructor | Parameters (all optional values shown with defaults) |
|---|---|
| `newCube()` | Unit cube. |
| `newBox(width=1, height=1, depth=1)` | Box dimensions. |
| `newSphere(latitudes=16, longitudes=24, radius=1)` | UV sphere. |
| `newPlane(subdivisions=1, size=1)` | Procedural plane in the XZ plane with +Y-facing normals. |
| `newCylinder(slices=24, radius=1, height=1)` | Cylinder. |
| `newCone(slices=24, radius=1, height=1)` | Cone. |
| `newCapsule(slices=24, rings=8, radius=0.5, height=1)` | Capsule. |
| `newTorus(segments=32, rings=16, majorRadius=1, minorRadius=0.3)` | Torus. |

### Materials

Materials expose:

- `setColor(r, g, b, [a])` / `getColor()` — base color and opacity; alpha defaults to 1.
- `setDiffuseTexture(texture)` / `getDiffuseTexture()`
- `setNormalTexture(texture)` / `getNormalTexture()`
- `setSpecularTexture(texture)` / `getSpecularTexture()`
- `setEmissiveTexture(texture)` / `getEmissiveTexture()`
- `setSpecularParams(strength, shininess)` / `getSpecularParams()`
- `setEmissiveColor(r, g, b)` / `getEmissiveColor()`
- `setPBR(boolean)`, `setMetallic(value)` / `getMetallic()`, and `setRoughness(value)` / `getRoughness()` — opt into the metallic-roughness direct-light model. Metallic must be between 0 and 1; roughness must be between 0.04 and 1.
- `setMetallicRoughnessTexture(texture)` and `setOcclusionTexture(texture)` — bind linear-data maps; glTF uses the standard blue metallic/green roughness channels and red occlusion channel. `setOcclusionStrength(value)` / `getOcclusionStrength()` sets the occlusion mix from 0 to 1.
- `setUnlit(boolean)` / `isUnlit()`
- `setTransparent(boolean)` / `isTransparent()`
- `setCullMode(mode)` / `getCullMode()` — LOVE cull mode, commonly `back`.
- `setShader(shader)` / `getShader()` — use or retrieve a custom LOVE shader for this material; pass `nil` to restore the module shader.
- `setFragmentFunction(source)` — compile a standard-material fragment hook. `source` must define `vec4 love3d_CustomFragment(vec4 color)` and may return a modified final shaded color.
- `reloadShader([source])` — recompile the stored hook, or replace it with the supplied source and recompile. Call outside `begin3d`/`end3d`; compilation errors are reported.

Pass `nil` to a texture setter to clear that texture slot. The lighting model is Blinn-Phong. Normal maps require usable UV coordinates; tangent frames are read from imported glTF or generated from geometry and UVs. Material opacity below 1 or vertex-color alpha routes a draw through the transparent pass. Diffuse texture alpha is not inspected to classify a draw, so call `setTransparent(true)` when a diffuse texture contains transparency. Transparency uses per-draw sorting, not order-independent transparency, so intersecting transparent surfaces may still sort imperfectly.

`setAlphaMode("auto"|"opaque"|"mask"|"blend")` selects the alpha behavior. `mask` discards fragments below `setAlphaCutoff(value)` (default 0.5); `blend` uses the sorted transparent pass. `setAdditiveBlend(true)` selects additive blending for a transparent material.

The fragment hook is injected after standard lighting and alpha calculation. It preserves the standard vertex inputs and LOVING3D material uniforms; it is not a complete custom vertex-shader or render-pipeline API.

### Scene graph, instancing, batching, and LOD

```lua
local root = g3d.newNode()
root:setPosition(3, 0, 0)
local child = g3d.newNode(mesh)
child:setPosition(0, 2, 0)
root:addChild(child)

g3d.begin3d()
g3d.draw(root)
g3d.end3d()
```

`Node3D` provides `setParent`/`getParent`, `addChild`/`removeChild`/`getChild`, local `setPosition`/`setRotation`/`setScale`, `setMesh`, and `getWorldPosition`. Parent cycles are rejected. A node may attach one mesh or model and child nodes; drawing a root recursively queues its subtree.

For repeated transforms, pass an array of records in either `{x,y,z, rx,ry,rz, sx,sy,sz}` form or a 16-number column-major matrix form:

```lua
g3d.begin3d()
g3d.drawInstanced(mesh, {
	{-2, 0, 0, 0, 0, 0, 1, 1, 1},
	{ 2, 0, 0, 0, 45, 0, 1, 1, 1},
})
g3d.end3d()
```

For lower allocation overhead, a `love.data.ByteData` may hold tightly packed float32 `mat4` matrices (16 floats / 64 bytes each, column-major). Invalid byte lengths and non-finite transforms raise errors. Basic LOD is configured with `mesh:setLOD(distance, replacementMesh)`; distance thresholds must be positive. The farthest matching threshold is selected independently per drawn mesh/model part.

`demos/graphics3d-stress` displays 5,000 objects and compares an explicitly unbatched `draw` loop with `drawInstanced`. Run `./scripts/benchmark-graphics3d.sh` to print the average frame time over 600 frames in each mode. The script requires the `loving3d-build` Docker image and an available display.

### Lights

At most eight lights can be active at once. Light types are `directional`, `point`, and `spot`.

Methods: `getType()`, `setPosition(x,y,z)` / `getPosition()`, `setDirection(x,y,z)` / `getDirection()`, `setColor(r,g,b)` / `getColor()`, `setIntensity(value)` / `getIntensity()`, `setRange(distance)` / `getRange()`, `setSpotAngles(innerDegrees, outerDegrees)` / `getSpotAngles()`, and `setFalloff(value)` / `getFalloff()`.

Direction is the direction a directional/spot light points into the scene. Position and range apply to point/spot lights; spot angles apply only to spot lights. Light RGB setter values are normalized display-space (sRGB-like) values converted to linear by the standard shader. The `getSpotAngles` getter returns inner and outer degrees.

### Models and OBJ/MTL import

```lua
local model, materials = g3d.newModel(objText, [mtlText], [textureMap])
```

The function returns a `Model3D` and an array of its unique `Material3D` objects. `textureMap` is optional and maps the exact texture filename parsed from MTL to a `love.graphics` texture object, usually an `Image`:

```lua
local image = love.graphics.newImage("models/fighter.png")
local objText = assert(love.filesystem.read("models/fighter.obj"))
local mtlText = assert(love.filesystem.read("models/fighter.mtl"))
local model, materials = g3d.newModel(objText, mtlText, {
	["fighter.png"] = image,
})
```

Supported OBJ data: `v`, `vt`, `vn`, `f`, `g`, `o`, and `usemtl`; face indices may be positive or negative and polygons are fan-triangulated. Missing normals are generated. Supported MTL values include `Kd`, `Ka`, `Ke`, `Ks`, `Ns`, `d`, `Tr`, `map_Kd`, `map_Ks`, `map_Ke`, `map_bump`, `bump`, and `Norm`. `illum` and smoothing groups are ignored. The source-text `newModel` does not read `mtllib`; use `newModelFromFile("path/to/model.obj")` to resolve its libraries and texture paths relative to each file. Texture map options beyond the filename are not interpreted. Model objects expose `getPartCount()`.

OBJ texture coordinates use a bottom-left origin, while LOVE image sampling uses a top-left origin. The importer flips V as `1 - v`; do not pre-flip OBJ UVs a second time.

### Color space

Material base/emissive colors, fog colors, and light colors are passed as normalized display-space (sRGB-like) values and converted to linear light by the standard shader. Scene ambient color is consumed directly as linear light. glTF base-color/emissive factors are converted from glTF's linear values before the shader conversion; metallic-roughness and occlusion textures are loaded as linear data. Other vertex colors and sampled textures are multiplied as sampled; the 3D shader does not explicitly decode their RGB values. Alpha is not gamma-converted. The shader relies on LOVE's gamma-correct framebuffer behavior for output encoding.

This is a focused OBJ/MTL importer, not a general-purpose DCC interchange pipeline. Validate asset winding, UVs, normals, material assignments, and scale in the target build.

### glTF 2.0 and GLB

`g3d.newModelFromFile(path)` dispatches by `.obj`, `.gltf`, or `.glb` extension. File reads and external resources use LOVE's virtual filesystem, so paths are relative to the game's mounted filesystem. The loader imports triangle primitives, indexed or non-indexed geometry, positions, normals (generated when omitted), UV set 0, vertex colors, tangents (generated when omitted), node matrix/TRS hierarchy flattened into part transforms, external and embedded buffers, and external or bufferView images. Base-color, normal, and emissive maps/factors, `OPAQUE` / `MASK` / `BLEND`, alpha cutoff, and glTF sampler wrap/filter settings are mapped to materials.

glTF metallic-roughness factors and metallic-roughness/occlusion maps are imported into the direct-light PBR path. Sparse accessors, non-triangle primitive modes, texture coordinate sets other than `TEXCOORD_0`, animation/skin/morph data, and all glTF extensions are unsupported. Unsupported `extensionsUsed` or `extensionsRequired` entries raise an error naming the extension; sparse accessors likewise fail with a specific error instead of being misread. glTF's linear base-color/emissive factors are converted for the existing material color path. Base-color/emissive images use the color-texture path; normal, metallic-roughness, and occlusion maps use linear data texture paths. Tangent handedness is adjusted for the V flip used by this renderer.

Run the interactive viewer with `./build/love demos/graphics3d-gltf-viewer`. It ships with the small Khronos Box GLB and accepts a relative model path as a launch argument; drag to orbit, use the wheel to zoom, and press Enter to type a replacement path. Automated loader fixtures and their individual license notices are under `testing/assets/gltf/`. The upstream sample collection currently contains `NormalTangentTest` rather than the requested `NormalTilingTest`.

Run `./build/love demos/graphics3d-fog-pbr` for the interactive fog and material grid. Press Space to cycle between Blinn-Phong, linear fog, exponential fog, and direct-light metallic-roughness PBR. Launch with `--benchmark` to measure 600 frames per mode at 1080p and print renderer information and average/minimum/maximum frame times.

The recorded 1080p benchmark on Mesa llvmpipe (OpenGL 4.5, 25 spheres, 600 frames per mode) reported:

| Mode | Average (ms) | Minimum (ms) | Maximum (ms) |
|---|---:|---:|---:|
| Blinn-Phong baseline | 10.466 | 0.122 | 220.838 |
| Linear fog | 10.180 | 6.173 | 15.254 |
| Exponential fog | 10.020 | 6.073 | 15.329 |
| Metallic-roughness PBR | 10.118 | 6.250 | 16.771 |

These are whole-frame measurements, not GPU timer-query measurements. The low minima and high baseline maximum show substantial frame-time noise; differences between these modes are not a reliable isolated GPU cost estimate.

## Current scope and limitations

The current module provides a camera, mesh/model resources, procedural shapes, OBJ/MTL and partial glTF/GLB loading, Blinn-Phong and direct-light metallic-roughness material shading with a fragment-function hook, up to eight lights, depth testing, sorted transparency, linear/exponential distance fog, a transform scene graph, GPU instancing, automatic opaque batching, and distance-based mesh LOD. Cubemap/procedural skyboxes, shadow mapping, image-based lighting, skeletal animation, 3D physics, and an extensible render graph are not currently provided. These are not implied by the API in this guide.

Backend, driver, and platform behavior should be validated on the actual shipping targets. A successful build or demo run on one machine does not certify performance, portability, or production readiness on other configurations. Keep 3D draw submission and resource creation measured for your own workload; `getStats()` exposes basic draw counts, not a full profiler.

## Troubleshooting

- **Depth looks incorrect or objects do not occlude:** enable `t.window.depth = true` for the main window, and ensure the render target supports depth.
- **Nothing is visible:** verify `setCamera` was called, the camera faces the geometry, the geometry lies within near/far clipping planes, and `begin3d`/`end3d` enclose the draw calls.
- **Faces disappear:** check triangle winding, material cull mode, and `setWinding("ccw")` / `"cw"`.
- **Text-based OBJ model has no texture:** supply the MTL text and a `textureMap` entry keyed by the exact `map_Kd` filename. Path-based OBJ and glTF imports resolve texture files relative to the source file.
- **Transparency overlaps incorrectly:** material opacity and vertex alpha are recognized; for diffuse texture alpha, mark the material transparent. Sorting is per draw, not per triangle.
- **Camera queries disagree with a Canvas view:** pass the Canvas pixel dimensions and set the camera aspect to the Canvas's width divided by height.
- **Debug helpers do nothing or error:** call `debugLine`, `debugBox`, `debugSphere`, and `debugNormals` between `begin3d` and `end3d`.

## Runnable demos

- [`demos/graphics3d-canvas`](../demos/graphics3d-canvas/) renders a 3D scene into a readable Canvas and composites it in the 2D pass.
- [`demos/graphics3d-resize`](../demos/graphics3d-resize/) updates camera aspect on resize; press **F** for fullscreen, **B** for bounds, and **N** for normals.
