# Agentic Coding, End-to-End (Rust Jumpman Clone Example)

`Rust: cargo run`

This repo contains a small Jumpman-style platformer with a primary Rust + Macroquad implementation in `mario-rust/`. It also includes a deterministic C++ port in `mario-cpp/` and a Python/Pygame version in `mario-python/`. This README uses the Rust codebase as a concrete example of **agentic coding**: a workflow where an AI agent plans work, makes changes with tools, evaluates results, and iterates until the software is ready to ship.

## Quickstart (Rust)

```bash
cd mario-rust
cargo run
```

- Controls + level format: see `mario-rust/README.md`
- C++ port: see `mario-cpp/README.md`
- Python version: see `mario-python/README.md`

## What “agentic coding” means

Agentic coding treats the model as a software teammate that can:

1. **Conceptualize**: clarify goals, constraints, and “definition of done”.
2. **Plan**: break work into small, verifiable steps.
3. **Execute**: change code/doc/tests and run local checks.
4. **Evaluate**: compare results to acceptance criteria (not vibes).
5. **Iterate + Ship**: tighten the loop until the project is releasable.

The key difference vs. “chatting about code”: every step is anchored in the repo, in files, with explicit constraints and checks.

---

## 1) Conceptualization (turn a vibe into a buildable target)

Start by writing a tight objective and boundaries.

**Example (Rust Jumpman clone):**

- **Objective:** a playable 1-level platformer with a title screen, a goal condition, scoring, enemies, and a restart loop.
- **Constraints:** Rust stable; no copyrighted Nintendo assets; keep dependencies minimal (this project uses `macroquad`).
- **Non-goals (for v1):** world editor, multiple biomes, save system, network play.
- **Acceptance criteria (vertical slice):**
  - Title → Playing → LevelComplete loop exists (`mario-rust/src/game/mod.rs`).
  - The level loads from ASCII (`mario-rust/assets/levels/level1.txt`) and has a fallback if it fails (`mario-rust/src/game/world.rs`).
  - Player movement feels “platformer-ish” (accel/decel, coyote time, jump buffer, jump cut) (`mario-rust/src/game/player.rs`, `mario-rust/src/game/mod.rs`).
  - Build + run instructions are documented (`mario-rust/README.md`).

**Architecture sketch (derived from the code):**

- **Game loop:** `mario-rust/src/main.rs` drives `update()` + `draw()` in a loop.
- **State machine:** `GameState` in `mario-rust/src/game/mod.rs` (Title/Playing/LevelComplete).
- **Data-driven content:** ASCII levels in `mario-rust/assets/levels/`.
- **“Feel” tuning:** `Config` in `mario-rust/src/game/mod.rs` centralizes physics constants.
- **Separation of concerns:** `world.rs`, `player.rs`, `enemy.rs`, `audio.rs`, `physics.rs`, etc.

If any of these are unclear, resolve that *before* writing code: unclear goals produce confident-but-wrong diffs.

---

## 2) AGENTS.md initialization (make the repo “agent-friendly”)

`AGENTS.md` is a short, repo-local contract: rules and context an agent should follow automatically.

This repo already has `AGENTS.md` at the root. In general, a strong `AGENTS.md` includes:

- **How to work:** small diffs, verify behavior, don’t assume missing context.
- **Project constraints:** e.g., “no copyrighted assets”, “avoid new dependencies”, “run `cargo fmt`”.
- **Where truth lives:** file paths for configs, entry points, and domain rules.
- **Definition of done:** what “finished” means for a change.

**Pattern: one root file + optional subproject files**

- Put global rules in root `AGENTS.md`.
- Add more specific `AGENTS.md` files inside subprojects when needed (e.g., `mario-rust/`) to encode local conventions like “physics constants live in `Config`”.

**Minimal example (template you can adapt):**

```md
# AGENTS.md (example)
- Prefer small, reviewable diffs.
- Ask clarifying questions when behavior is underspecified.
- For `mario-rust/`: keep tuning constants in `src/game/mod.rs:Config`.
- Validate with: `cargo fmt`, `cargo test`, `cargo run` (as applicable).
- Do not add copyrighted Jumpman assets.
```

---

## 3) Prompting (turn goals into executable tasks)

A good prompt is basically a mini-spec. Include:

- **Where:** exact directory/files (e.g., `mario-rust/src/game/player.rs`).
- **What:** the desired behavior.
- **Constraints:** dependencies, performance, style, gameplay feel.
- **Acceptance criteria:** how you’ll confirm it worked.
- **How to verify:** commands to run.

**Prompt template**

```text
In `mario-rust/`, implement <feature>.

Context:
- Game loop is Macroquad; `Game` is in `src/game/mod.rs`.
- Levels load from `assets/levels/*.txt` via `World::load` in `src/game/world.rs`.

Constraints:
- Keep changes minimal; no new dependencies.
- Keep tuning constants in `Config` (`src/game/mod.rs`).

Acceptance criteria:
- <bullet list of observable outcomes>

Verify:
- `cargo fmt`
- `cargo run`
```

**Example prompts (using the existing code layout)**

1) *Add a second level and a level select flow*

- Update `mario-rust/assets/levels/` with `level2.txt`.
- Add `GameState::LevelSelect` (or a simpler approach) in `mario-rust/src/game/mod.rs`.
- Ensure `World::load()` is called with the selected level path.
- Acceptance: pressing a key on title lets you choose a level, then play it.

2) *Add a “coyote time” tuning HUD for feel iteration*

- Expose `Config::coyote_time` and `Config::jump_buffer_time` in a debug overlay.
- Acceptance: values render on screen; changing constants changes feel predictably.

The agent should propose a plan, implement the smallest working slice, and only then iterate on polish.

---

## 4) Iterative development (tight loops, fast learning)

Agentic iteration is: **one hypothesis → one diff → one verification → repeat**.

**A practical iteration sequence for the Jumpman clone**

1. **Make it run:** window + main loop (`mario-rust/src/main.rs`).
2. **Make it playable:** player movement + collisions (`mario-rust/src/game/player.rs`, `mario-rust/src/game/physics.rs`).
3. **Make it a game:** world/goal/scoring (`mario-rust/src/game/world.rs`, `mario-rust/src/game/mod.rs`).
4. **Add threats + rewards:** enemies, coins, mushrooms (`mario-rust/src/game/enemy.rs`, `mario-rust/src/game/world.rs`).
5. **Polish:** camera/background/audio/HUD (`mario-rust/src/game/background.rs`, `mario-rust/src/game/audio.rs`).

**How the agent evaluates each step**

- Compares results to acceptance criteria (e.g., “coin increments score by 200”).
- Runs a local verification command (often `cargo run` for a game).
- Keeps gameplay constants centralized (in this project, `Config` in `mario-rust/src/game/mod.rs`) so tuning doesn’t sprawl.

If the behavior is subjective (“jump feels floaty”), define a measurable surrogate (jump height/time, terminal velocity, accel/decel, etc.) and iterate with small changes.

---

## 5) Shipping (turn “works on my machine” into a deliverable)

Shipping is mostly checklist work:

1. **Release build:** `cd mario-rust && cargo build --release`
2. **Asset bundling:** this project uses `set_pc_assets_folder("assets")` (`mario-rust/src/game/mod.rs`), so the shipped binary must be able to find `assets/` at runtime (typically by distributing the `assets/` folder alongside the executable and running from that directory).
3. **Docs:** keep `mario-rust/README.md` accurate (controls, level format, audio overrides).
4. **Sanity pass:** play through level end-to-end; verify restart/quit flows.
5. **Versioning:** tag releases and record changes (e.g., changelog) if you’re distributing builds.

---

## Repo layout

```text
.
├── AGENTS.md
├── mario-cpp/
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── assets/
│   ├── core/
│   ├── app/
│   └── tests/
├── mario-python/
│   ├── README.md
│   └── src/
└── mario-rust/
    ├── Cargo.toml
    ├── README.md
    ├── assets/
    │   └── levels/level1.txt
    └── src/
        ├── main.rs
        └── game/
            ├── mod.rs
            ├── world.rs
            ├── player.rs
            ├── enemy.rs
            ├── physics.rs
            ├── audio.rs
            ├── sprites.rs
            └── background.rs
```

### Appendix

- During the process of development, the agent exhibited emergent behaviors, such as including invincibility frames after taking damage, which were not explicitly requested but improved gameplay.
- The agent also designed the power-up player sprite to look like Luigi instead of Jumpman, showcasing creativity within the constraints provided.
- All assets used are original, produced by the agent.
# mario
