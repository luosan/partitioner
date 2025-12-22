#!/bin/bash

# Build script for standalone TritonPart

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building Standalone TritonPart${NC}"
echo "=================================="

# Check if OpenSTA is available
if [ ! -d "lib/sta" ]; then
    echo -e "${YELLOW}Warning: OpenSTA source not found in lib/sta${NC}"
    echo "Please copy or link OpenSTA source to lib/sta"
    echo "Example: ln -s /path/to/OpenSTA lib/sta"
    exit 1
fi

# Create build directory
if [ -d "build" ]; then
    echo "Cleaning existing build directory..."
    rm -rf build
fi
mkdir -p build
cd build

# Configure with CMake
echo -e "${GREEN}Configuring with CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DUSE_ORTOOLS=ON \
    2>&1 | tee cmake.log

# Build
echo -e "${GREEN}Building...${NC}"
make -j$(nproc) 2>&1 | tee build.log

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${NC}"
    echo ""
    echo "Executable: build/bin/tritonpart"
    echo "Libraries: build/lib/"
    echo ""
    echo "To run tests:"
    echo "  cd build && ctest"
else
    echo -e "${RED}Build failed!${NC}"
    echo "Check build/build.log for details"
    exit 1
fi
