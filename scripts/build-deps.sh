#!/usr/bin/env bash
#
# LOVING3D - safe install/remove protocol for the build dependencies.
#
# The danger with build dependencies is never the install (they are leaf dev
# libraries); it is running `apt autoremove` blindly afterwards, which can
# sweep up core system packages that were left flagged "auto". This script
# makes every removal step explicit and reviewed:
#
#   install: snapshots the manual-package list, installs the deps, saves the
#            exact diff as the future removal list, then SIMULATES autoremove
#            so any surprise is visible immediately.
#
#   remove:  purges exactly the saved list, then SIMULATES autoremove and
#            hard-aborts if any core system package appears in the plan.
#
# Usage:
#   ./scripts/build-deps.sh install
#   ./scripts/build-deps.sh remove
##

set -euo pipefail

PKGS=(
	cmake
	libsdl3-dev
	libharfbuzz-dev
	libopenal-dev
	libmodplug-dev
	libtheora-dev
	libvorbis-dev
	libogg-dev
	libluajit-5.1-dev
)

# Packages that must never be swept up by an autoremove. If any of these show
# up in a simulated autoremove, the script stops and tells you how to protect
# them instead of proceeding.
DANGER='gnome-shell|gdm3|network-manager|linux-image|linux-generic|linux-headers|xorg|ubuntu-desktop|ubuntu-minimal|ubuntu-standard|mesa-'

SNAPDIR="$HOME/.cache/loving3d-build-deps"
BEFORE="$SNAPDIR/apt-manual-before.txt"
AFTER="$SNAPDIR/apt-manual-after.txt"
ADDED="$SNAPDIR/apt-packages-added.txt"

snapshot() {
	mkdir -p "$SNAPDIR"
	apt-mark showmanual | sort > "$BEFORE"
}

# Prints the simulated autoremove plan and returns non-zero if it touches
# anything in $DANGER. Read-only: -s never changes the system.
check_autoremove_plan() {
	local plan
	plan="$(apt-get -s autoremove | grep '^Remv' || true)"

	if [[ -z "$plan" ]]; then
		echo "Simulated autoremove: nothing would be removed. Safe."
		return 0
	fi

	if echo "$plan" | grep -qE "$DANGER"; then
		echo "WARNING: autoremove would remove CORE SYSTEM packages:"
		echo "$plan" | grep -E "$DANGER"
		echo
		echo "Protect them first (flag flip only, installs nothing):"
		echo "  sudo apt-mark manual <those packages>"
		echo "Then re-run this script. Nothing was removed."
		return 1
	fi

	echo "Simulated autoremove plan (dev libraries only):"
	echo "$plan"
	return 0
}

case "${1:-}" in
install)
	snapshot
	sudo apt-get install -y "${PKGS[@]}"
	apt-mark showmanual | sort > "$AFTER"
	# Lines only present after the install = the packages you added.
	comm -13 "$BEFORE" "$AFTER" > "$ADDED"

	echo
	echo "Added (your removal list, saved to $ADDED):"
	cat "$ADDED"
	echo
	check_autoremove_plan
	;;

remove)
	if [[ ! -s "$ADDED" ]]; then
		echo "No saved removal list at $ADDED."
		echo "Run this script's 'install' first, or purge the packages manually."
		exit 1
	fi

	echo "Purging exactly the packages added by install:"
	cat "$ADDED"
	sudo apt-get purge -y $(cat "$ADDED")

	echo
	if check_autoremove_plan; then
		read -r -p "Run the real 'apt-get autoremove' now? [y/N] " answer
		if [[ "$answer" == "y" || "$answer" == "Y" ]]; then
			sudo apt-get autoremove
		else
			echo "Aborted - nothing was autoremoved."
		fi
	else
		exit 1
	fi
	;;

*)
	echo "Usage: $0 {install|remove}"
	echo "  install - snapshot, install the build deps, save the removal list"
	echo "  remove  - purge the saved list, then a reviewed autoremove"
	exit 1
	;;
esac
