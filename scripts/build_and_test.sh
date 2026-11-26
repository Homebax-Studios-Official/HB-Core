#!/usr/bin/env bash
# Configure, build and test HB Core on Linux. Usage: scripts/build_and_test.sh [debug|release]
set -euo pipefail
cd "$(dirname "$0")/.."
kind="${1:-release}"
cmake --preset "linux-${kind}"
cmake --build --preset "linux-${kind}"
ctest --preset "linux-${kind}"
python3 tools/verify_tree.py
