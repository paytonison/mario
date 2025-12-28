# Jumpman: Platformer Porting Trilogy (Python → Rust → C++)

A small, finished platformer built three times—then **ported forward** from an easier language to a harder one while keeping the game spec consistent. This repo exists to prove one thing clearly: I can take a project from **cradle → shipped** and then do disciplined, behavior-preserving ports across stacks.

## What this demonstrates

- Shipping a complete vertical slice (title → play → win/lose → restart).
- Porting the same game forward **Python → Rust → C++** while preserving core behavior.
- Practical engineering loop: scope → implement → debug → document → package.
- A clean sandbox for agent tooling + automated evaluation (build/run correctness, regression cases).

## Implementations

Each directory is a full, runnable version:

1. `mario-python/` — Python + Pygame (original implementation)
2. `mario-rust/` — Rust + Macroquad (port of the Python version)
3. `mario-cpp/` — C++20 (port of the Rust version)

(Directory names keep the historical prefix; the project itself is a **generic platformer**. No Nintendo assets are used.)

## Quickstart

- Python: `cd mario-python && python src/main.py`  
  See `mario-python/README.md` for setup + controls.

- Rust: `cd mario-rust && cargo run`  
  See `mario-rust/README.md` for controls + level format.

- C++: `cd mario-cpp && mkdir -p build && cd build && cmake .. && cmake --build . && ./jumpman`  
  See `mario-cpp/README.md` for platform notes.

## Porting philosophy

The point of the trilogy is not “three unrelated implementations.” It’s **one spec**, carried forward:

- Same high-level loop: title → playing → level complete / death → restart.
- Same world representation: ASCII level input (where supported) and equivalent collision rules.
- Same “feel” targets: acceleration/deceleration, gravity, jump arc, coyote time / jump buffer (if implemented).
- Same gameplay contracts: enemies hurt you, power-up changes player state, score increments are consistent.

What is allowed to change per port:
- The rendering/input framework (Pygame vs Macroquad vs custom C++ loop).
- Data layout and architecture (idiomatic Python vs Rust ownership/borrowing vs C++ RAII).
- Performance/memory choices and build tooling.

## How to read this as a portfolio project

If you only have two minutes:

- Start in `mario-python/` to see the original design and the “fast iteration” version.
- Jump to `mario-rust/` to see the first serious port under stricter constraints.
- Finish with `mario-cpp/` to see the hardest version and the cleanest separation of “core” from “app”.

The important signal is not novelty. It’s execution: finish a game, then port it twice without collapsing the spec.

## Using this repo as an eval harness (agent + tooling)

This repo is intentionally shaped to be easy to evaluate:

- Objective graders: “does it build?”, “does it run?”, “do tests pass?”, “does the level load?”, “does the replay reach LevelComplete?”
- Natural regression set: every bug/edge case becomes a new test case or dataset item.
- Realistic agent tasks: fix a build break, implement a feature behind acceptance criteria, refactor without behavior drift.

Suggested harness pattern:
1. Collect real tasks (bug reports, feature tickets, refactors) into a small dataset.
2. Define graders that are as non-subjective as possible:
   - compile/build succeeds
   - unit/integration tests pass (where present)
   - schema/format checks (level files, config) pass
   - optional: deterministic input replay reaches expected checkpoints
3. Run the same dataset against different prompts/models/tooling configs and compare deltas.
4. Promote any failure into a permanent regression case.

If you’re using OpenAI’s Evals tooling: this project is a clean “ground truth” target because build/test outcomes are hard signals rather than vibes.

## Repository layout

```text
jumpman/
├── mario-python/
│   ├── README.md
│   └── src/
├── mario-rust/
│   ├── README.md
│   ├── Cargo.toml
│   ├── assets/
│   └── src/
└── mario-cpp/
    ├── README.md
    ├── CMakeLists.txt
    ├── assets/
    ├── core/
    ├── app/
    └── tests/
```

## Agent-assisted development (what “agentic coding” means here)

I used an agent as a teammate, but not as a magic wand. The loop was:

- Write a tight objective + constraints + acceptance criteria.
- Ask for a small plan (small diffs, verifiable steps).
- Implement one slice.
- Verify locally (build/run/tests).
- Iterate until the spec is met, then document how to run it.

The agent is a productivity multiplier; the proof of correctness is the repo itself: buildable, runnable, and readable.

## Assets and IP

- No copyrighted Nintendo assets.
- All included assets are original (produced during development for this project).

## Appendix: notable emergent behavior during development

- The agent introduced small gameplay improvements (e.g., i-frames after damage) that were not explicitly requested but improved playability.
- The powered-up player sprite ended up as an “alternate palette” character—a cute artifact of the process.

---

If you’re reviewing this for engineering ability: treat it like a systems porting exercise with a game as the substrate.
