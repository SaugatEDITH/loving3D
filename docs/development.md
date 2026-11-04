# Build, run, and test LOVING3D

This repository is a modified LÖVE source tree. Build the fork's `love`
executable before running games that use `love.graphics3d`; an unmodified
LÖVE installation does not include this module.

## Linux build

### Docker toolchain (recommended)

Install Docker, then run this from the repository root:

```sh
./scripts/docker-demo.sh build
```

The script builds the `loving3d-build` image on its first run, mounts the
repository into the container, and writes build output to `./build/`. Later
builds reuse the image and existing CMake build directory.

### Native CMake build

Install CMake, a C/C++ toolchain, and the development libraries required by
the repository's CMake configuration. Configure and compile:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
```

If dependency discovery fails, install the missing development package and
rerun the configure command. The exact dependencies vary by distribution; the
repository's [Dockerfile](../docker/Dockerfile) lists the Linux packages used
by its container toolchain.

## Run a demo or game

Run one of the checked-in demos with the binary you just built:

```sh
./build/love demos/graphics3d-cube
./build/love demos/graphics3d-gltf-viewer
./build/love demos/graphics3d-fog-pbr
./build/love demos/graphics3d-shooter
./build/love demos/graphics3d-stress
./build/love demos/rubiks3d
```

The executable accepts either a game directory or a `.love` archive. Replace
the demo path with your project path, for example:

```sh
./build/love ../my-game
./build/love ../my-game.love
```

See [Creating a game](creating-a-game.md) for a starter project and packaging
steps.

## Run tests

Run all engine tests, or narrow the run while developing:

```sh
./build/love testing --all
./build/love testing --modules graphics3d
./build/love testing --modules graphics3d,graphics
./build/love testing --method graphics3d ModelFiles
```

The suite creates reports in `testing/output/` (`lovetest_all.md`,
`lovetest_all.html`, and `lovetest_all.xml` for a full run), with rendered
image comparisons under `testing/output/actual/` and
`testing/output/difference/`. A graphics-capable display and renderer are
needed for the rendering tests. Test output can vary by operating system,
GPU, and graphics backend; a pass on one configuration is not certification
for every target.

`--isRunner` enables the suite's CI-runner-specific tolerance and skip
behavior; use it only when reproducing the repository's CI environment:

```sh
./build/love testing --all --isRunner
```

See [testing/readme.md](../testing/readme.md) for test organization, assertions,
and report details.

## Validate and package your own game

1. Follow [Creating a game](creating-a-game.md) to create `conf.lua` and
   `main.lua`.
2. Run the project with `./build/love path/to/game` while iterating.
3. Keep assets inside the game directory and load them through
   `love.filesystem`.
4. To create a `.love` archive, place `main.lua` and `conf.lua` at the archive
   root and zip the project contents:

   ```sh
   cd path/to/game
   zip -r ../my-game.love .
   cd -
   ```

5. Test the archive with `./build/love path/to/my-game.love`.

The archive is the game source package; shipping a standalone application
requires bundling it with the appropriate LOVING3D engine build for each
target platform. Follow that platform's packaging and licensing requirements.
