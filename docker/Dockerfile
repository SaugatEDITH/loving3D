#
# LOVING3D - throwaway build/test environment for the love.graphics3d module.
#
# Copyright (c) 2026 Saugat Pokharel
#
# This file is part of LOVING3D, an independent extension adding native 3D
# capabilities to the LOVE game engine (love2d.org). The underlying engine
# remains freely distributable under the terms of its original zlib license
# (see license.txt).
#
# The image contains only the build toolchain and dev libraries. The source
# tree is bind-mounted at run time (not copied), and the build output lands
# in the project's build/ directory on the host. Removing the image with
#   docker rmi loving3d-build
# returns the system to its prior state - nothing is installed on the host.
#

FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
	&& apt-get install -y --no-install-recommends \
		build-essential \
		cmake \
		pkg-config \
		libsdl3-dev \
		libharfbuzz-dev \
		libopenal-dev \
		libmodplug-dev \
		libtheora-dev \
		libvorbis-dev \
		libogg-dev \
		libluajit-5.1-dev \
		libgl-dev \
		libvulkan1 \
		libgl1 \
		libegl1 \
		libgl1-mesa-dri \
		mesa-vulkan-drivers \
		gdb \
	&& rm -rf /var/lib/apt/lists/*
