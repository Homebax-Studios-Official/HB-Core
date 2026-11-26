#!/usr/bin/env bash
# Builds Release and installs the SDK (headers, libhbcore, CMake package) into ./dist.
set -euo pipefail
cd "$(dirname "$0")/.."
cmake --preset linux-release
cmake --build --preset linux-release
cmake --install build/linux-release --prefix dist
echo "SDK installed to $(pwd)/dist"
