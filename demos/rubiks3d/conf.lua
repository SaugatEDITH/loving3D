--
-- LOVING3D - Rubik's Cube capability demo configuration.
--
-- Copyright (c) 2026 Saugat Pokharel
--
-- This file is part of LOVING3D, an independent extension adding native 3D
-- capabilities to the LOVE game engine (love2d.org). The underlying engine
-- remains freely distributable under the terms of its original zlib license
-- (see license.txt).
--

function love.conf(t)
	t.identity = "loving3d-rubiks-demo"
	t.window.title = "LOVING3D - Rubik's Cube"
	t.window.width = 1280
	t.window.height = 720
	t.window.vsync = 1
	t.window.msaa = 4
	t.window.resizable = true

	-- REQUIRED for love.graphics3d on the main screen: the window needs a
	-- depth buffer, otherwise begin3d fails with an informative error.
	t.window.depth = true
end
