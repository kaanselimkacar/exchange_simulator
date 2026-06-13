# Build Instructions

## Prerequisites
- CMake 3.15+
- GCC 10+ or Clang 10+
- Google Test (via nix flake)
- (Optional) `clang-tidy`, `clang-format`

## Quick Start

### Debug Build (with Tests)
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```
Tests automatically build and run on every Debug build.

### Release Build
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## Tests

Tests are in `tests/domain/`. Debug builds auto-build and run tests (via CTest), which
reports the test suite as a single pass/fail. To see every individual `TEST_F` case:

```bash
cd build
./tests/market_tests
```

## Code Quality

All warnings as errors. Format code:
```bash
cd build
cmake --build . --target format
```
