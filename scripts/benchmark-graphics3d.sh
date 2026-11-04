#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

args=(--rm --user "$(id -u):$(id -g)" -e HOME=/tmp -e SDL_AUDIODRIVER=dummy
	-v "$PWD:/src" -w /src)
if [[ -n "${DISPLAY:-}" && -d /tmp/.X11-unix ]]; then
	args+=(-e "DISPLAY=${DISPLAY}" -v /tmp/.X11-unix:/tmp/.X11-unix)
	if [[ -n "${XAUTHORITY:-}" && -f "${XAUTHORITY:-}" ]]; then
		args+=(-e XAUTHORITY=/tmp/.Xauthority -v "$XAUTHORITY:/tmp/.Xauthority:ro")
	fi
fi

docker run "${args[@]}" loving3d-build ./build/love demos/graphics3d-stress --benchmark
