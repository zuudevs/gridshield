#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
BUILD_TYPE="${1:-Release}"

echo "═══════════════════════════════════════════════════════════"
echo "  GridShield Build Script"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "Build Type: ${BUILD_TYPE}"
echo "Project Directory: ${PROJECT_DIR}"
echo "Build Directory: ${BUILD_DIR}"
echo ""

# Create build directory
if [ -d "${BUILD_DIR}" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure
echo "Configuring CMake..."
cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" ..

# Build
echo "Building..."
cmake --build . -j$(nproc)

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  Build Complete!"
echo "  Executable: ${PROJECT_DIR}/bin/gridshield"
echo "═══════════════════════════════════════════════════════════"
echo ""