--
-- LOVING3D - native 3D space shooter stress demo.
--
-- Copyright (c) 2026 Saugat Pokharel
--
-- This file is part of LOVING3D, an independent extension adding native 3D
-- capabilities to the LOVE game engine (love2d.org). The underlying engine
-- remains freely distributable under the terms of its original zlib license
-- (see license.txt).
--

function love.conf(t)
	t.identity = "loving3d-space-shooter"
	t.window.title = "LOVING3D - Starfall"
	t.window.width = 1000
	t.window.height = 700
	t.window.vsync = 1
	t.window.resizable = true
	t.window.depth = true
end
