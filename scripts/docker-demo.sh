#!/usr/bin/env bash
#
# LOVING3D - build the engine and run the Rubik's Cube demo inside a
# throwaway Docker container. The source tree is bind-mounted (never copied),
# the build output goes to ./build on the host, and the image can be deleted
# afterwards with: docker rmi loving3d-build
#
# Usage:
#   ./scripts/docker-demo.sh          # build image if needed, build, run demo
#   ./scripts/docker-demo.sh build    # only build the engine (no demo)
#   ./scripts/docker-demo.sh shell    # interactive shell inside the container
##

set -euo pipefail
cd "$(dirname "$0")/.."

IMAGE=loving3d-build

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
	echo "Building image $IMAGE (one-time, ~1 GB)..."
	docker build -t "$IMAGE" -f docker/Dockerfile .
fi

# Run as the host user so build artifacts are not owned by root, with a
# writable HOME inside the container. -t requires a TTY, which background
# invocations don't have.
RUN_ARGS=(
	--user "$(id -u):$(id -g)"
	-e HOME=/tmp
	-v "$PWD:/src"
	-w /src
)

if [[ -t 0 ]]; then
	RUN_ARGS+=(--rm -it)
else
	RUN_ARGS+=(--rm -i)
fi

# GPU access for hardware-accelerated GL/Vulkan (falls back to software
# rendering inside the container if this is not available).
if [[ -d /dev/dri ]]; then
	RUN_ARGS+=(--device /dev/dri)
fi

# Display: X11/XWayland socket (GNOME on Wayland provides this too).
if [[ -n "${DISPLAY:-}" && -d /tmp/.X11-unix ]]; then
	RUN_ARGS+=(-e "DISPLAY=${DISPLAY}" -v /tmp/.X11-unix:/tmp/.X11-unix)
	if [[ -n "${XAUTHORITY:-}" && -f "${XAUTHORITY:-}" ]]; then
		RUN_ARGS+=(-e XAUTHORITY=/tmp/.Xauthority -v "$XAUTHORITY:/tmp/.Xauthority:ro")
	fi
fi

# Wayland as a fallback when there is no X11 socket at all.
if [[ -z "${DISPLAY:-}" && -n "${WAYLAND_DISPLAY:-}" ]]; then
	RUN_ARGS+=(
		-e "WAYLAND_DISPLAY=${WAYLAND_DISPLAY}"
		-e "XDG_RUNTIME_DIR=/tmp/runtime"
		-v "${XDG_RUNTIME_DIR:-/run/user/$(id -u)}:/tmp/runtime"
	)
fi

case "${1:-run}" in
build)
	docker run "${RUN_ARGS[@]}" "$IMAGE" bash -c '
		cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
		cmake --build build -j"$(nproc)"
	'
	;;
shell)
	docker run "${RUN_ARGS[@]}" "$IMAGE" bash
	;;
debug)
	# Build with debug info, then run under gdb and print a backtrace on crash.
	docker run "${RUN_ARGS[@]}" "$IMAGE" bash -c '
		cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
		cmake --build build -j"$(nproc)"
		gdb -batch -ex "run" -ex "bt 25" -ex "info registers eip rip" --args ./build/love demos/rubiks3d
	'
	;;
run)
	docker run "${RUN_ARGS[@]}" "$IMAGE" bash -c '
		cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
		cmake --build build -j"$(nproc)"
		exec stdbuf -oL -eL ./build/love demos/rubiks3d
	'
	;;
*)
	echo "Usage: $0 {run|build|debug|shell}" >&2
	exit 1
	;;
esac
