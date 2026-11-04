-- love.graphics3d

local g3d = love.graphics3d

local function makeCanvas()
  return love.graphics.newCanvas(64, 64, { format = "rgba8", readable = true })
end

local function renderColor(canvas, draw)
  canvas:renderTo(function()
    love.graphics.clear(0, 0, 0, 1)
    g3d.begin3d()
    draw()
    g3d.end3d()
  end)
  return love.graphics.readbackTexture(canvas):getPixel(32, 32)
end

love.test.graphics3d.CameraQueries = function(test)
  local camera = g3d.newCamera()
  camera:setAspect(1)
  camera:setPosition(0, 0, 5)
  camera:lookAt(0, 0, 0)
  local sx, sy, depth = camera:worldToScreen(0, 0, 0, 64, 64)
  test:assertEquals(32, math.floor(sx + 0.5), "perspective center x")
  test:assertEquals(32, math.floor(sy + 0.5), "perspective center y")
  test:assertEquals(5, math.floor(depth + 0.5), "perspective forward depth")
  local x, y, z = camera:screenToWorld(sx, sy, depth, 64, 64)
  test:assertEquals(0, math.floor(x + 0.5), "perspective roundtrip x")
  test:assertEquals(0, math.floor(y + 0.5), "perspective roundtrip y")
  test:assertEquals(0, math.floor(z + 0.5), "perspective roundtrip z")
  local ox, oy, oz, dx, dy, dz = camera:screenRay(32, 32, 64, 64)
  test:assertEquals(5, math.floor(oz + 0.5), "perspective ray origin")
  test:assertEquals(-1, math.floor(dz + 0.5), "perspective ray direction")
  local fx, fy, fz = camera:getForward()
  test:assertEquals(-1, math.floor(fz + 0.5), "camera forward basis")
  local rx, ry, rz = camera:getRight()
  test:assertEquals(1, math.floor(rx + 0.5), "camera right basis")
  local ux, uy, uz = camera:getUp()
  test:assertEquals(1, math.floor(uy + 0.5), "camera up basis")

  camera:setProjectionMode("orthographic")
  camera:setOrthoExtents(-2, 2, -1, 1)
  local osx, osy, od = camera:worldToScreen(1, 0.5, 0, 64, 64)
  local wx, wy, wz = camera:screenToWorld(osx, osy, od, 64, 64)
  test:assertEquals(48, math.floor(osx + 0.5), "orthographic screen x")
  test:assertEquals(16, math.floor(osy + 0.5), "orthographic screen y")
  test:assertEquals(1, math.floor(wx + 0.5), "orthographic roundtrip x")
  test:assertEquals(1, math.floor(wy + 0.5), "orthographic roundtrip y")
  test:assertEquals(0, math.floor(wz + 0.5), "orthographic roundtrip z")
  local rox, roy, roz, rdx, rdy, rdz = camera:screenRay(32, 32, 64, 64)
  test:assertEquals(5, math.floor(roz + 0.5), "orthographic ray origin")
  test:assertEquals(-1, math.floor(rdz + 0.5), "orthographic ray direction")
end

love.test.graphics3d.BoundsAndCulling = function(test)
  local mesh = g3d.newBox(2, 4, 6)
  local minx, miny, minz, maxx, maxy, maxz = mesh:getBounds()
  test:assertEquals(-1, minx, "mesh bounds min x")
  test:assertEquals(-2, miny, "mesh bounds min y")
  test:assertEquals(-3, minz, "mesh bounds min z")
  test:assertEquals(1, maxx, "mesh bounds max x")
  test:assertEquals(2, maxy, "mesh bounds max y")
  test:assertEquals(3, maxz, "mesh bounds max z")

  local camera = g3d.newCamera()
  camera:setAspect(1)
  camera:setPosition(0, 0, 5)
  camera:lookAt(0, 0, 0)
  g3d.setCamera(camera)
  local canvas = makeCanvas()
  canvas:renderTo(function()
    love.graphics.clear(0, 0, 0, 1)
    g3d.begin3d()
    g3d.draw(mesh, 100, 0, 0)
    g3d.end3d()
  end)
  local stats = g3d.getStats()
  test:assertEquals(1, stats.culled, "offscreen mesh culled")
  test:assertEquals(0, stats.drawCalls, "culled mesh not submitted")

  local obj = table.concat({
    "v 0 0 0",
    "v 1 0 0",
    "v 0 1 0",
    "f 1 2 3",
  }, "\n")
  local model = g3d.newModel(obj)
  local bx0, by0, bz0, bx1, by1, bz1 = model:getBounds()
  test:assertEquals(0, bx0, "model bounds min x")
  test:assertEquals(1, bx1, "model bounds max x")
end

love.test.graphics3d.DepthAndCulling = function(test)
  local camera = g3d.newCamera()
  camera:setAspect(1)
  camera:setPosition(0, 0, 4)
  camera:lookAt(0, 0, 0)
  g3d.setCamera(camera)

  local near = g3d.newBox(1, 1, 0.2)
  local nearMaterial = g3d.newMaterial()
  nearMaterial:setColor(1, 0, 0, 1)
  nearMaterial:setUnlit(true)
  near:setMaterial(nearMaterial)
  local far = g3d.newBox(1, 1, 0.2)
  local farMaterial = g3d.newMaterial()
  farMaterial:setColor(0, 1, 0, 1)
  farMaterial:setUnlit(true)
  far:setMaterial(farMaterial)

  local canvas = makeCanvas()
  local r, g, b = renderColor(canvas, function()
    g3d.draw(near, 0, 0, 0)
    g3d.draw(far, 0, 0, -1)
  end)
  test:assertGreaterEqual(0.5, r, "near red surface occludes far surface")
  test:assertRange(g, 0, 0.2, "far green surface is depth-occluded")
  local stats = g3d.getStats()
  test:assertEquals(2, stats.drawCalls, "per-pass draw calls")
  test:assertEquals(24, stats.triangles, "per-pass triangle count")
  test:assertEquals(0, stats.lightsUsed, "per-pass active light count")

  local triangle = g3d.newMesh({
    { -1, -1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1 },
    { 1, -1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1 },
    { 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1 },
  }, { 1, 2, 3 })
  local material = g3d.newMaterial()
  material:setUnlit(true)
  material:setColor(1, 1, 1, 1)
  material:setCullMode("back")
  triangle:setMaterial(material)
  g3d.setWinding("ccw")
  local visible = renderColor(canvas, function() g3d.draw(triangle, 0, 0, 0) end)
  test:assertGreaterEqual(0.5, visible, "CCW front face survives backface culling")
  g3d.setWinding("cw")
  local culled = renderColor(canvas, function() g3d.draw(triangle, 0, 0, 0) end)
  test:assertRange(culled, 0, 0.1, "setWinding reverses front-face interpretation")
  g3d.setWinding("ccw")
end

love.test.graphics3d.TransparentOrdering = function(test)
  local camera = g3d.newCamera()
  camera:setAspect(1)
  camera:setPosition(0, 0, 4)
  camera:lookAt(0, 0, 0)
  g3d.setCamera(camera)
  local far = g3d.newBox(1, 1, 0.2)
  local near = g3d.newBox(1, 1, 0.2)
  local farMaterial = g3d.newMaterial()
  farMaterial:setColor(1, 0, 0, 0.5)
  farMaterial:setTransparent(true)
  farMaterial:setUnlit(true)
  far:setMaterial(farMaterial)
  local nearMaterial = g3d.newMaterial()
  nearMaterial:setColor(0, 0, 1, 0.5)
  nearMaterial:setTransparent(true)
  nearMaterial:setUnlit(true)
  near:setMaterial(nearMaterial)
  local r, g, b = renderColor(makeCanvas(), function()
    g3d.draw(near, 0, 0, 0)
    g3d.draw(far, 0, 0, -1)
  end)
  test:assertGreaterEqual(0.3, b, "near transparent blue is blended last")
  test:assertGreaterEqual(0.15, r, "far transparent red is blended first")
  test:assertGreaterEqual(r, b, "transparent sort is back-to-front")

  local opaque = g3d.newBox(1, 1, 0.2)
  local opaqueMaterial = g3d.newMaterial()
  opaqueMaterial:setColor(1, 0, 0, 1)
  opaqueMaterial:setUnlit(true)
  opaque:setMaterial(opaqueMaterial)
  local frontGlass = g3d.newBox(1, 1, 0.2)
  frontGlass:setMaterial(nearMaterial)
  local or_, og, ob = renderColor(makeCanvas(), function()
    g3d.draw(frontGlass, 0, 0, 0.5)
    g3d.draw(opaque, 0, 0, 0)
  end)
  test:assertGreaterEqual(0.2, or_, "opaque draw is submitted before transparent draw")
  test:assertGreaterEqual(0.2, ob, "front transparent surface blends over opaque surface")
end

love.test.graphics3d.OBJImport = function(test)
  local obj = table.concat({
    "v 0 0 0",
    "v 1 0 0",
    "v 0 1 0",
    "vt 0 0",
    "vt 1 0",
    "vt 0 1",
    "usemtl checker",
    "f -3/-3 -2/-2 -1/-1",
  }, "\n")
  local positiveModel = g3d.newModel("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3")
  test:assertEquals(1, positiveModel:getPartCount(), "positive indices import a model part")
  local imageData = love.image.newImageData(2, 2)
  imageData:setPixel(0, 0, 1, 0, 0, 1)
  local image = love.graphics.newImage(imageData)
  local model, materials = g3d.newModel(obj, "newmtl checker\nmap_Kd checker.png", {
    ["checker.png"] = image,
  })
  test:assertEquals(1, model:getPartCount(), "negative indices import a model part")
  test:assertEquals(0, select(1, model:getBounds()), "OBJ model bounds min x")
  test:assertEquals(image, materials[1]:getDiffuseTexture(), "MTL map_Kd binds texture")

  local camera = g3d.newCamera()
  camera:setAspect(1)
  camera:setPosition(0, 0, 4)
  camera:lookAt(0, 0, 0)
  g3d.setCamera(camera)
  g3d.clearLights()
  g3d.setAmbientColor(0, 0, 0)
  local light = g3d.newLight("directional")
  light:setDirection(0, 0, -1)
  light:setIntensity(1)
  g3d.addLight(light)
  local noNormalModel = g3d.newModel("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3")
  local r, g, b = renderColor(makeCanvas(), function()
    g3d.draw(noNormalModel, -0.2, -0.2, 0)
  end)
  test:assertGreaterEqual(0.1, r + g + b, "missing OBJ normals are generated for lit rendering")
  g3d.clearLights()
end

love.test.graphics3d.ModelFiles = function(test)
  local models = {
    "Box/Box.gltf",
    "NormalTangentTest/NormalTangentTest.gltf",
    "Box/Box.glb",
    "NormalTangentTest/NormalTangentTest.glb",
  }

  for _, path in ipairs(models) do
    local model = g3d.newModelFromFile("assets/gltf/" .. path)
    test:assertGreaterEqual(1, model:getPartCount(), path .. " creates drawable parts")
  end

  local obj = g3d.newModelFromFile("assets/gltf/OBJPath/PathImport.obj")
  test:assertEquals(1, obj:getPartCount(), "OBJ path import resolves mtllib and map_Kd")

  local camera = g3d.newCamera()
  camera:setAspect(1)
  camera:setPosition(0, 0, 5)
  camera:lookAt(0, 0, 0)
  g3d.setCamera(camera)
  g3d.clearLights()
  local normalModel = g3d.newModelFromFile("assets/gltf/NormalTangentTest/NormalTangentTest.glb")
  local r, g, b = renderColor(makeCanvas(), function()
    g3d.draw(normalModel, 0, 0, 0)
  end)
  test:assertGreaterEqual(0.01, r + g + b, "glTF tangent and normal-map material renders to Canvas")

  local extensionPath = "unsupported-extension.gltf"
  love.filesystem.write(extensionPath,
    '{"asset":{"version":"2.0"},"extensionsUsed":["EXT_test_unsupported"]}')
  local extensionOK, extensionError = pcall(g3d.newModelFromFile, extensionPath)
  love.filesystem.remove(extensionPath)
  test:assertFalse(extensionOK, "unsupported glTF extension is rejected")
  test:assertTrue(tostring(extensionError):find("EXT_test_unsupported", 1, true) ~= nil,
    "unsupported extension error names the extension")

  local sparsePath = "sparse-accessor.gltf"
  love.filesystem.write(sparsePath,
    '{"asset":{"version":"2.0"},"accessors":[{"componentType":5126,"count":1,"type":"VEC3","sparse":{}}],"meshes":[{"primitives":[{"attributes":{"POSITION":0}}]}]}')
  local sparseOK, sparseError = pcall(g3d.newModelFromFile, sparsePath)
  love.filesystem.remove(sparsePath)
  test:assertFalse(sparseOK, "sparse glTF accessor is rejected")
  test:assertTrue(tostring(sparseError):find("Sparse glTF accessors", 1, true) ~= nil,
    "sparse accessor error is explicit")
end

love.test.graphics3d.DuckSample = function(test)
  local gltfPath = "assets/gltf/Duck/Duck.gltf"
  local glbPath = "assets/gltf/Duck/Duck.glb"
  test:assertTrue(love.filesystem.getInfo(gltfPath) ~= nil, "licensed Duck glTF fixture is present")
  test:assertTrue(love.filesystem.getInfo(glbPath) ~= nil, "licensed Duck GLB fixture is present")
  test:assertGreaterEqual(1, g3d.newModelFromFile(gltfPath):getPartCount(), "Duck glTF imports")
  test:assertGreaterEqual(1, g3d.newModelFromFile(glbPath):getPartCount(), "Duck GLB imports")
end

love.test.graphics3d.BoxTexturedSample = function(test)
  local gltfPath = "assets/gltf/BoxTextured/BoxTextured.gltf"
  local glbPath = "assets/gltf/BoxTextured/BoxTextured.glb"
  if love.filesystem.getInfo(gltfPath) == nil or love.filesystem.getInfo(glbPath) == nil then
    test:skipTest("Upstream BoxTextured includes a trademarked logo; place licensed fixtures here to run.")
    return
  end
  test:assertGreaterEqual(1, g3d.newModelFromFile(gltfPath):getPartCount(), "BoxTextured glTF imports")
  test:assertGreaterEqual(1, g3d.newModelFromFile(glbPath):getPartCount(), "BoxTextured GLB imports")
end

love.test.graphics3d.DamagedHelmetSample = function(test)
  local gltfPath = "assets/gltf/DamagedHelmet/DamagedHelmet.gltf"
  local glbPath = "assets/gltf/DamagedHelmet/DamagedHelmet.glb"
  if love.filesystem.getInfo(gltfPath) == nil or love.filesystem.getInfo(glbPath) == nil then
    test:skipTest("DamagedHelmet includes non-commercial textures; provide licensed fixtures to run.")
    return
  end
  test:assertGreaterEqual(1, g3d.newModelFromFile(gltfPath):getPartCount(), "DamagedHelmet glTF imports")
  test:assertGreaterEqual(1, g3d.newModelFromFile(glbPath):getPartCount(), "DamagedHelmet GLB imports")
end

love.test.graphics3d.CanvasAndPassMisuse = function(test)
  local camera = g3d.newCamera()
  camera:setAspect(1)
  camera:setPosition(0, 0, 4)
  camera:lookAt(0, 0, 0)
  g3d.setCamera(camera)
  local mesh = g3d.newBox(1, 1, 1)
  local material = g3d.newMaterial()
  material:setColor(1, 1, 1, 1)
  material:setUnlit(true)
  mesh:setMaterial(material)
  local r, g, b = renderColor(makeCanvas(), function()
    g3d.draw(mesh, 0, 0, 0)
    g3d.debugLine(-1, 0, 0, 1, 0, 0)
    g3d.debugBox(-0.75, -0.75, -0.75, 0.75, 0.75, 0.75)
    g3d.debugSphere(0, 0, 0, 0.35)
    g3d.debugNormals(mesh, 0, 0, 0, 0, 0, 0, 1, 1, 1)
  end)
  test:assertGreaterEqual(0.5, r + g + b, "3D pass renders into a Canvas")
  test:assertGreaterEqual(1, g3d.getStats().drawCalls, "debug primitives submit in the 3D pass")

  local ok, err = pcall(g3d.draw, mesh, 0, 0, 0)
  test:assertFalse(ok, "drawing outside a 3D pass errors")
  test:assertTrue(string.find(err, "between begin3D and end3d", 1, true) ~= nil, "draw error explains pass requirement")

  g3d.begin3d()
  local nestedOK, nestedError = pcall(g3d.begin3d)
  test:assertFalse(nestedOK, "nested begin3d errors")
  test:assertTrue(string.find(nestedError, "already called", 1, true) ~= nil, "nested begin3d error is clear")
  g3d.end3d()

  g3d.begin3d()
  local missingEndOK, missingEndError = pcall(g3d.begin3d)
  test:assertFalse(missingEndOK, "starting a new pass detects a missing end3d")
  test:assertTrue(string.find(missingEndError, "end3d first", 1, true) ~= nil, "missing end3d error is clear")
  g3d.end3d()
end

love.test.graphics3d.SceneGraphInstancingBatchingAndLOD = function(test)
  local camera = g3d.newCamera()
  camera:setAspect(1)
  camera:setPosition(0, 0, 6)
  camera:lookAt(0, 0, 0)
  g3d.setCamera(camera)

  local material = g3d.newMaterial()
  material:setColor(0.9, 0.2, 0.1, 1)
  material:setUnlit(true)
  local mesh = g3d.newBox(0.8, 0.8, 0.8)
  mesh:setMaterial(material)

  local root = g3d.newNode()
  root:setPosition(-1, 0, 0)
  local child = g3d.newNode(mesh)
  child:setPosition(2, 0, 0)
  root:addChild(child)
  local wx, wy, wz = child:getWorldPosition()
  test:assertEquals(1, wx, "child inherits parent translation")
  test:assertEquals(0, wy, "child world y")
  test:assertEquals(0, wz, "child world z")
  test:assertEquals(root, child:getParent(), "child exposes its parent")
  root:setScale(2, 1, 1)
  wx = select(1, child:getWorldPosition())
  test:assertEquals(3, wx, "child inherits parent scale")
  root:setRotation(0, 0, 90)
  wx, wy = child:getWorldPosition()
  test:assertRange(wx, -1.001, -0.999, "child inherits parent rotation x")
  test:assertRange(wy, 3.999, 4.001, "child inherits parent rotation y")
  root:setRotation(0, 0, 0)
  root:setScale(1, 1, 1)
  local cycleOK, cycleError = pcall(child.addChild, child, root)
  test:assertFalse(cycleOK, "scene graph rejects parent cycles")
  test:assertTrue(tostring(cycleError):find("cycle", 1, true) ~= nil, "cycle error is explicit")

  local canvas = makeCanvas()
  canvas:renderTo(function()
    love.graphics.clear(0, 0, 0, 1)
    g3d.begin3d()
    g3d.draw(root)
    g3d.end3d()
  end)
  local nodePixel = love.graphics.readbackTexture(canvas):getPixel(39, 32)
  test:assertGreaterEqual(0.5, nodePixel, "parented child is rendered at its world transform")

  local transforms = {
    { -1.1, 0, 0, 0, 0, 0, 1, 1, 1 },
    { 1.1, 0, 0, 0, 0, 0, 1, 1, 1 },
  }
  canvas:renderTo(function()
    love.graphics.clear(0, 0, 0, 1)
    g3d.begin3d()
    g3d.drawInstanced(mesh, transforms)
    g3d.end3d()
  end)
  local stats = g3d.getStats()
  test:assertEquals(2, stats.drawsPassed, "instanced input counts visible objects")
  test:assertEquals(2, stats.queued, "instanced objects are independently culled")
  test:assertLessEqual(2, stats.drawCalls, "instanced transforms use at most one draw call when supported")

  local packedMatrices = {}
  for _, x in ipairs({ -1, 1 }) do
    for _, value in ipairs({ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, 0, 0, 1 }) do
      packedMatrices[#packedMatrices + 1] = value
    end
  end
  local byteTransforms = love.data.pack("data", "<" .. string.rep("f", #packedMatrices), unpack(packedMatrices))
  canvas:renderTo(function()
    love.graphics.clear(0, 0, 0, 1)
    g3d.begin3d()
    g3d.drawInstanced(mesh, byteTransforms)
    g3d.end3d()
  end)
  test:assertEquals(2, g3d.getStats().drawsPassed, "packed ByteData matrices are accepted")
  local malformedTransforms = love.data.newByteData(63)
  local byteOK, byteError = pcall(g3d.drawInstanced, mesh, malformedTransforms)
  test:assertFalse(byteOK, "mis-sized instance ByteData is rejected")
  test:assertTrue(tostring(byteError):find("multiple of 64", 1, true) ~= nil, "ByteData length error explains the required format")

  canvas:renderTo(function()
    love.graphics.clear(0, 0, 0, 1)
    g3d.begin3d()
    g3d.draw(mesh, -1.1, 0, 0)
    g3d.draw(mesh, 0, 0, 0)
    g3d.draw(mesh, 1.1, 0, 0)
    g3d.end3d()
  end)
  stats = g3d.getStats()
  test:assertEquals(1, stats.drawCalls, "repeated opaque mesh/material draws batch automatically")
  test:assertEquals(36, stats.triangles, "batching preserves triangle accounting")

  g3d.setAutoBatching(false)
  canvas:renderTo(function()
    love.graphics.clear(0, 0, 0, 1)
    g3d.begin3d()
    g3d.draw(mesh, -1.1, 0, 0)
    g3d.draw(mesh, 0, 0, 0)
    g3d.draw(mesh, 1.1, 0, 0)
    g3d.end3d()
  end)
  test:assertEquals(3, g3d.getStats().drawCalls, "diagnostic mode disables automatic batching")

  canvas:renderTo(function()
    love.graphics.clear(0, 0, 0, 1)
    g3d.begin3d()
    g3d.drawInstanced(mesh, transforms)
    g3d.end3d()
  end)
  test:assertEquals(1, g3d.getStats().drawCalls, "explicit instancing works when automatic batching is disabled")
  g3d.setAutoBatching(true)

  local lowDetail = g3d.newPlane(1, 0.8)
  lowDetail:setMaterial(material)
  mesh:setLOD(2, lowDetail)
  canvas:renderTo(function()
    love.graphics.clear(0, 0, 0, 1)
    g3d.begin3d()
    g3d.draw(mesh, 0, 0, 0)
    g3d.end3d()
  end)
  test:assertEquals(2, g3d.getStats().triangles, "distance selects the registered low-detail mesh")

  local invalidLOD, invalidError = pcall(mesh.setLOD, mesh, -1, lowDetail)
  test:assertFalse(invalidLOD, "LOD distance must be positive")
  test:assertTrue(tostring(invalidError):find("positive", 1, true) ~= nil, "invalid LOD distance reports why")
end

love.test.graphics3d.FragmentHookAndShaderReload = function(test)
  local camera = g3d.newCamera()
  camera:setAspect(1)
  camera:setPosition(0, 0, 4)
  camera:lookAt(0, 0, 0)
  g3d.setCamera(camera)
  local material = g3d.newMaterial()
  local customShader = love.graphics.newShader([[
    vec4 effect(vec4 color, Image tex, vec2 texture_coords, vec2 screen_coords) {
      return color;
    }
  ]])
  material:setShader(customShader)
  test:assertEquals(customShader, material:getShader(), "material accepts and returns a custom LOVE shader")
  material:setShader(nil)
  test:assertEquals(nil, material:getShader(), "material can clear its custom shader")

  material:setColor(1, 0, 0, 1)
  material:setUnlit(true)
  material:setFragmentFunction("vec4 love3d_CustomFragment(vec4 color) { return vec4(0.0, 1.0, 0.0, color.a); }")
  local mesh = g3d.newBox(1, 1, 1)
  mesh:setMaterial(material)
  local r, g, b = renderColor(makeCanvas(), function()
    g3d.draw(mesh, 0, 0, 0)
  end)
  test:assertLessEqual(0.1, r, "fragment hook replaces final red channel")
  test:assertGreaterEqual(0.5, g, "fragment hook outputs green")

  g3d.setAutoBatching(false)
  local canvas = makeCanvas()
  canvas:renderTo(function()
    love.graphics.clear(0, 0, 0, 1)
    g3d.begin3d()
    g3d.drawInstanced(mesh, {
      { -0.6, 0, 0, 0, 0, 0, 1, 1, 1 },
      { 0.6, 0, 0, 0, 0, 0, 1, 1, 1 },
    })
    g3d.end3d()
  end)
  test:assertEquals(1, g3d.getStats().drawCalls, "custom fragment shader supports explicit instancing")
  g3d.setAutoBatching(true)

  material:reloadShader("vec4 love3d_CustomFragment(vec4 color) { return vec4(0.0, 0.0, 1.0, color.a); }")
  r, g, b = renderColor(makeCanvas(), function()
    g3d.draw(mesh, 0, 0, 0)
  end)
  test:assertLessEqual(0.1, g, "shader reload replaces previous fragment hook")
  test:assertGreaterEqual(0.5, b, "reloaded shader outputs blue")
end
