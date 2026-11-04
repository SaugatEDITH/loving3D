# LOVING3D documentation

LOVING3D is a LÖVE-based engine distribution. Use the documentation for the API area you are working with:

- [LÖVE 2D and shared framework APIs](https://love2d.org/wiki/Main_Page) — official documentation for the existing `love.graphics`, `love.window`, `love.filesystem`, input, audio, and other LÖVE APIs. This repository does not replace their behavior or document them a second time.
- [LOVING3D graphics guide](graphics3d.md) — the added `love.graphics3d` module, setup, rendering lifecycle, API reference, model import, and constraints.
- [Creating a game](creating-a-game.md) — starter project structure, 3D setup, running, testing, and packaging a `.love` archive.
- [Build, run, and test](development.md) — Linux build options, demo commands, test selectors, and generated reports.
- [Test suite guide](../testing/readme.md) — test organization, assertions, graphics tolerance, and runner exceptions.
- [Licensing and attribution](licensing.md) — how to read the licenses and notices included with this LÖVE-based source distribution.

The guide describes the APIs exposed by this repository's current source. It is not a promise that every upstream LÖVE version, rendering backend, or operating system has been validated with the 3D module. Record and test the exact revision and target configuration used by your project.
