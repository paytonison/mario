#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include "mario/core/config.hpp"
#include "mario/core/game_state.hpp"
#include "mario/core/replay.hpp"
#include "mario/core/world.hpp"
#include "test_paths.hpp"

namespace fs = std::filesystem;

namespace {

bool read_file(const fs::path& path, std::string& out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return false;
  }
  in.seekg(0, std::ios::end);
  const std::streamoff size = in.tellg();
  if (size < 0) {
    return false;
  }
  out.resize(static_cast<std::size_t>(size));
  in.seekg(0, std::ios::beg);
  in.read(out.data(), size);
  return in.good();
}

}  // namespace

TEST_CASE("replay: golden replay produces stable end-state hash") {
  const mario::core::Config config{};

  const fs::path root = fs::path(mario::tests::kMarioCppRootDir);
  const fs::path replay_path = root / "tests" / "replays" / "golden_level1_v1.jsonl";

  std::string replay_contents;
  REQUIRE(read_file(replay_path, replay_contents));

  mario::core::Replay replay{};
  std::string error;
  REQUIRE(mario::core::replay_from_jsonl(replay_contents, replay, error));

  REQUIRE(!replay.level.empty());
  const fs::path level_path = root / "assets" / fs::path(replay.level);

  std::string level_contents;
  REQUIRE(read_file(level_path, level_contents));

  mario::core::World world{};
  REQUIRE(mario::core::World::from_ascii(level_contents, config, world, error));

  mario::core::GameState state = mario::core::make_new_game(std::move(world), config);
  for (const mario::core::StepInput& input : replay.inputs) {
    mario::core::step(state, input);
  }

  REQUIRE(state.tick == replay.inputs.size());
  REQUIRE(mario::core::hash_state(state) == 0x48dc25b3a530daf9ull);
}

