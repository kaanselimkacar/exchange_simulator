# ExchangeSimulator

A test-driven, exchange-agnostic simulation of an electronic order-matching
engine, written in C++23. The core matching logic is deliberately free of any
exchange-specific assumptions, so a completely new exchange can be plugged in
without touching the domain.

## Design Goals

- **No hardcoded exchange logic.** Exchange-specific details live behind a
  dedicated extension seam (`src/protocols/`), keeping the domain reusable.
- **Test-first development.** Tests are written (LLM-generated) before
  implementation; every build is gated by `-Werror` and clang-tidy.
- **Clean, typed error handling.** Domain operations report failures through
  `std::expected` and a rich `StatusCode` enum rather than exceptions.

## Development Process

Strict test-first workflow: every behavior is pinned down by a failing test
before any implementation lands. Tests are LLM-generated; all implementation
under `src/` is authored by the project owner.

## Current Status

Implemented:

- Price-level orderbook with **add / modify / delete** of orders
  (`Domain::Market`)
- Matching engine that crosses orders and produces trades
  (`Domain::MatchingEngine`)
- `DomainGateway` facade wiring orderbooks + matching engine together
- Typed status-code errors (`std::expected` / `StatusCode`) throughout
- spdlog-backed logging layer
- 133 GoogleTest cases across three test executables (LLM-generated;
  `src/` is authored by the project owner)
- C++23, strict compiler warnings (`-Werror`), clang-tidy as a build gate

## Architecture

```
src/
├── domain/        Exchange-agnostic core: POD order/trade types, orderbook,
│                  price levels, matching engine
├── common/        Shared infrastructure (spdlog-backed Logger)
├── protocols/     Per-exchange Config adapters (extension seam, WIP)
├── domain_gateway.cpp/hpp   Facade over market + matching engine
```

Domain code uses only POD primitives (`Order`, `Trade`, `Side`, plus numeric
aliases) and matching primitives (`Orderbook`, `PriceLevel`). No exchange
name or wire format is baked in.

## Build Instructions

Prerequisites: CMake 3.15+, a C++23 compiler, Google Test. A nix flake
(`flake.nix`) provides the full toolchain (clang, cmake, gtest, clang-tools).

### Debug build (builds and runs tests)

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

### Release build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## Tests

Tests live in `tests/domain/`. Debug builds auto-build and run the suite via
CTest (reported as a single pass/fail). To run the whole suite as one
aggregate result:

```bash
cd build
cmake --build . --target run_tests
```

To see every individual test case:

```bash
cd build
./tests/market_tests
./tests/matching_engine_tests
./tests/domain_gateway_tests
```

## Code Quality

All warnings are treated as errors, and clang-tidy findings fail the build.
Format source files with:

```bash
cd build
cmake --build . --target format
```

## Roadmap

- **Networking layer**: TCP order gateway + UDP market-data broadcast,
  built from small testable primitives (socket, IO buffer, frame parser,
  poller).
- **Protocol adapters**: per-exchange `Config` types under `src/protocols/`
  so new exchanges plug in via configuration alone.

## License

[MIT](LICENSE)