# glTF test assets

`Box` and `NormalTangentTest` are from
[KhronosGroup/glTF-Sample-Assets](https://github.com/KhronosGroup/glTF-Sample-Assets).
`Box` was created by Cesium (2017), under CC-BY-4.0. `NormalTangentTest` was
created by Ed Mackey / Analytical Graphics, Inc. (2018), under CC0-1.0.
Preserve the model-specific `LICENSE.md` files.

The Duck glTF/GLB sample was created by Sony (2006). Its SCEA Shared Source
License is included in `../LICENSES/SCEA.txt`, with the upstream notice in its
`LICENSE.md`.

The upstream collection currently has `NormalTangentTest`, not the requested
`NormalTilingTest`, so the suite uses the former for normal-map/tangent tests.
`BoxTextured` and `DamagedHelmet` fixtures are not redistributed here: the
former includes a trademarked logo and the latter includes non-commercial
textures. Their optional tests run when a user supplies those licensed models
at the documented fixture paths.

`OBJPath` is a small local regression fixture; its MTL points to the existing
LOVE test image using a relative path.
