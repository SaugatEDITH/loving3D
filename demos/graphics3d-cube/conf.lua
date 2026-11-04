--
-- LOVING3D - minimal 3D rendering demo.
--
-- Copyright (c) 2026 Saugat Pokharel
--
-- This file is part of LOVING3D, an independent extension adding native 3D
-- capabilities to the LOVE game engine (love2d.org). The underlying engine
-- remains freely distributable under the terms of its original zlib license
-- (see license.txt).
--

function love.conf(t)
	t.identity = "loving3d-cube-demo"
	t.window.title = "LOVING3D - 3D Cube"
	t.window.width = 800
	t.window.height = 600
	t.window.vsync = 1
	t.window.resizable = true
	t.window.depth = true
end
