# mario-cpp

Deterministic C++ port of the `mario-rust/` Jumpman-style platformer.

## Build (core + headless CLI + tests)

Requirements:
- CMake 3.20+
- A C++20 compiler (MSVC 2022, Clang, or GCC)

```bash
cmake -S mario-cpp -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Run (headless)

```bash
./build/bin/mario --headless --ticks 600 --assets-dir mario-cpp/assets
```

## Record / Replay

Record a replay (JSONL inputs):

```bash
./build/bin/mario --record out.jsonl --ticks 600 --assets-dir mario-cpp/assets
```

Replay and verify a hash:

```bash
./build/bin/mario --replay out.jsonl --assets-dir mario-cpp/assets --expect-hash 0xDEADBEEF
```

Golden replay (checked in):

```bash
./build/bin/mario --replay mario-cpp/tests/replays/golden_level1_v1.jsonl --assets-dir mario-cpp/assets --expect-hash 0x48dc25b3a530daf9
```

## Optional: SDL2 app

```bash
cmake -S mario-cpp -B build -DMARIO_CPP_BUILD_SDL_APP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/bin/mario_sdl --assets-dir mario-cpp/assets
```

