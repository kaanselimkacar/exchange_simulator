# Build Instructions

## Prerequisites
- CMake 3.15+
- GCC 10+ or Clang 10+
- (Optional) `clang-tidy`, `clang-format`

## Quick Start
```bash
# 1. Create build directory
mkdir build && cd build

# 2. Configure (Choose ONE compiler)
cmake .. -DCMAKE_CXX_COMPILER=clang++  # For Clang
cmake .. -DCMAKE_CXX_COMPILER=g++      # For GCC

# 3. Build
cmake --build .
