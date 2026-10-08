#!/usr/bin/env bash
set -e

mkdir -p docs/wasm

echo "Compiling Satsim C++ source files into WebAssembly..."
em++ -O3 -std=c++17 \
  -Iinclude \
  -fexceptions \
  --bind \
  src/*.cpp \
  src/*/*.cpp \
  -o docs/wasm/satsim.js

echo "Build complete! Output saved to docs/wasm/"
