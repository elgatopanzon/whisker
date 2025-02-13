#!/usr/bin/env bash

######################################################################
# @author      : ElGatoPanzon (contact@elgatopanzon.io)
# @file        : web
# @created     : Monday Feb 03, 2025 13:07:16 CST
#
# @description : 
######################################################################

set -euo pipefail

ROOT_DIR="$(pwd)"
BUILD_DIR="$ROOT_DIR/build.em"

# create build directory
if [ ! -d "$BUILD_DIR" ]; then
	mkdir "$BUILD_DIR"
fi

# prepare emsdk
cd "$BUILD_DIR"

# Get the emsdk repo
if [ ! -d emsdk ]; then
	git clone https://github.com/emscripten-core/emsdk.git
fi

# Enter that directory
cd emsdk

# Fetch the latest version of the emsdk (not needed the first time you clone)
git pull

# Download and install the latest SDK tools.
./emsdk install latest

# Make the "latest" SDK "active" for the current user. (writes .emscripten file)
./emsdk activate latest

# Activate PATH and other environment variables in the current terminal
source ./emsdk_env.sh

# build raylib for web
cd "$BUILD_DIR"

if [ ! -d raylib ]; then
	git clone https://github.com/raysan5/raylib
fi
cd raylib
git pull
emcmake cmake -S . -B build && cmake --build build

# Get into the /src folder
cd "$ROOT_DIR/src"

# Build to Web Assembly
emcc -o "$BUILD_DIR/index.html" \
    -I. -I *.h \
    *.c -Os -Wall "$BUILD_DIR/raylib/build/raylib/libraylib.a" \
    -I. -I "$BUILD_DIR/raylib/build/raylib/include" \
    -L. -L "$BUILD_DIR/raylib/src" \
    -s USE_GLFW=3 \
    -s ASYNCIFY \
    --shell-file ../shell.html \
    --preload-file ../assets \
    -s TOTAL_STACK=64MB \
    -s INITIAL_MEMORY=128MB \
    -s ASSERTIONS \
    -DPLATFORM_WEB
	
# Get into build dir
cd "$BUILD_DIR"

zip index.wasm.zip index.*

cd "$ROOT_DIR"

if [ $1 == "run" ]; then
	# Get into build dir
	cd "$BUILD_DIR"

	# Run the game
	emrun index.html
fi
