#!/usr/bin/env bash
set -e

BUILD_DIR="build"

if [ ! -d "$BUILD_DIR" ]; then
	mkdir -p "$BUILD_DIR"
fi


cmake -S . -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"

ln -sf $BUILD_DIR/compile_commands.json compile_commands.json
