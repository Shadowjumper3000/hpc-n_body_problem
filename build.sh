#!/bin/bash
# Build script for N-Body simulation
# Usage: ./build.sh [cpu|gpu|clean]

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default build type
BUILD_TYPE="Release"
USE_CUDA="OFF"
BUILD_DIR="build"

# Parse arguments
case "${1:-cpu}" in
    cpu)
        echo -e "${GREEN}Building CPU-only version${NC}"
        USE_CUDA="OFF"
        ;;
    gpu)
        echo -e "${GREEN}Building GPU-accelerated version${NC}"
        USE_CUDA="ON"
        ;;
    clean)
        echo -e "${YELLOW}Cleaning build directory${NC}"
        rm -rf $BUILD_DIR
        echo -e "${GREEN}Clean complete${NC}"
        exit 0
        ;;
    debug)
        echo -e "${YELLOW}Building debug version${NC}"
        BUILD_TYPE="Debug"
        ;;
    *)
        echo -e "${RED}Usage: $0 [cpu|gpu|clean|debug]${NC}"
        exit 1
        ;;
esac

# Check for required tools
echo "Checking dependencies..."

# Check CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}ERROR: CMake not found. Please install CMake 3.18+${NC}"
    exit 1
fi

# Check MPI
if ! command -v mpirun &> /dev/null && ! command -v mpiexec &> /dev/null; then
    echo -e "${RED}ERROR: MPI not found. Please load MPI module or install MPI${NC}"
    echo "Try: source env/modules.txt"
    exit 1
fi

# Check CUDA if GPU build requested
if [ "$USE_CUDA" == "ON" ]; then
    if ! command -v nvcc &> /dev/null; then
        echo -e "${RED}ERROR: CUDA not found. Please load CUDA module${NC}"
        echo "Try: module load cuda"
        exit 1
    fi
    echo -e "${GREEN}Found CUDA: $(nvcc --version | grep release | awk '{print $5}')${NC}"
fi

# Print configuration
echo ""
echo "========================================="
echo "Build Configuration:"
echo "  Build Type: $BUILD_TYPE"
echo "  CUDA Support: $USE_CUDA"
echo "  Build Directory: $BUILD_DIR"
echo "========================================="
echo ""

# Create build directory
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure with CMake
echo "Configuring..."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      -DUSE_CUDA=$USE_CUDA \
      -DUSE_OPENMP=ON \
      ..

if [ $? -ne 0 ]; then
    echo -e "${RED}Configuration failed!${NC}"
    exit 1
fi

# Build
echo ""
echo "Building..."
make -j$(nproc 2>/dev/null || echo 4)

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Success
echo ""
echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}Build successful!${NC}"
echo -e "${GREEN}=========================================${NC}"
echo ""
echo "Executable: $BUILD_DIR/nbody"
echo ""
echo "Quick test:"
echo "  mpirun -np 4 ./$BUILD_DIR/nbody --particles 1000 --steps 10"
echo ""
echo "For cluster runs, submit Slurm jobs in slurm/ directory"
