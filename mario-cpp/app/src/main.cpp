#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mario/core/config.hpp"
#include "mario/core/game_state.hpp"
#include "mario/core/replay.hpp"
#include "mario/core/world.hpp"

namespace fs = std::filesystem;

namespace {

struct Args {
  bool headless = false;
  std::optional<std::int64_t> ticks;
  fs::path assets_dir = "assets";
  std::string level = "levels/level1.txt";

  std::optional<fs::path> record_path;
  std::optional<fs::path> replay_path;
  std::optional<std::uint64_t> expect_hash;
};

void print_usage(std::ostream& os) {
  os << "Usage:\n"
     << "  mario --headless --ticks N [--assets-dir DIR] [--level PATH]\n"
     << "  mario --record PATH --ticks N [--assets-dir DIR] [--level PATH]\n"
     << "  mario --replay PATH [--ticks N] [--assets-dir DIR] [--expect-hash HEX]\n\n"
     << "Notes:\n"
     << "  - Core simulation is fixed-step at 60 Hz.\n"
     << "  - Replay format is JSONL; first line may contain {\"version\":1,\"level\":\"...\"}.\n";
}

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

bool write_file(const fs::path& path, std::string_view contents) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    return false;
  }
  out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
  return out.good();
}

bool parse_i64(std::string_view s, std::int64_t& out) {
  s = std::string_view{s.data(), s.size()};
  if (s.empty()) {
    return false;
  }
  const char* begin = s.data();
  const char* end = s.data() + s.size();
  std::int64_t v = 0;
  const auto res = std::from_chars(begin, end, v);
  if (res.ec != std::errc{} || res.ptr != end) {
    return false;
  }
  out = v;
  return true;
}

bool parse_u64_hex(std::string_view s, std::uint64_t& out) {
  if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
    s.remove_prefix(2);
  }
  if (s.empty()) {
    return false;
  }
  std::uint64_t v = 0;
  const auto res = std::from_chars(s.data(), s.data() + s.size(), v, 16);
  if (res.ec != std::errc{} || res.ptr != s.data() + s.size()) {
    return false;
  }
  out = v;
  return true;
}

bool parse_args(int argc, char** argv, Args& out, bool& wants_help) {
  wants_help = false;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    auto require_value = [&](std::string_view flag) -> std::optional<std::string_view> {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << flag << "\n";
        return std::nullopt;
      }
      i += 1;
      return std::string_view{argv[i]};
    };

    if (arg == "--help" || arg == "-h") {
      print_usage(std::cout);
      wants_help = true;
      return false;
    }
    if (arg == "--headless") {
      out.headless = true;
      continue;
    }
    if (arg == "--ticks") {
      auto v = require_value(arg);
      if (!v.has_value()) {
        return false;
      }
      std::int64_t ticks = 0;
      if (!parse_i64(*v, ticks) || ticks < 0) {
        std::cerr << "Invalid --ticks value\n";
        return false;
      }
      out.ticks = ticks;
      continue;
    }
    if (arg == "--assets-dir") {
      auto v = require_value(arg);
      if (!v.has_value()) {
        return false;
      }
      out.assets_dir = fs::path{std::string(*v)};
      continue;
    }
    if (arg == "--level") {
      auto v = require_value(arg);
      if (!v.has_value()) {
        return false;
      }
      out.level = std::string(*v);
      continue;
    }
    if (arg == "--record") {
      auto v = require_value(arg);
      if (!v.has_value()) {
        return false;
      }
      out.record_path = fs::path{std::string(*v)};
      continue;
    }
    if (arg == "--replay") {
      auto v = require_value(arg);
      if (!v.has_value()) {
        return false;
      }
      out.replay_path = fs::path{std::string(*v)};
      continue;
    }
    if (arg == "--expect-hash") {
      auto v = require_value(arg);
      if (!v.has_value()) {
        return false;
      }
      std::uint64_t expected = 0;
      if (!parse_u64_hex(*v, expected)) {
        std::cerr << "Invalid --expect-hash value\n";
        return false;
      }
      out.expect_hash = expected;
      continue;
    }

    std::cerr << "Unknown argument: " << arg << "\n";
    return false;
  }

  return true;
}

bool load_world_or_fallback(const mario::core::Config& config, const fs::path& level_path,
                           mario::core::World& out_world) {
  std::string contents;
  std::string error;
  if (read_file(level_path, contents)) {
    if (mario::core::World::from_ascii(contents, config, out_world, error)) {
      return true;
    }
    std::cerr << "Level parse error: " << error << ". Using fallback level.\n";
  } else {
    std::cerr << "Level load error: " << level_path.string() << ". Using fallback level.\n";
  }

  if (!mario::core::World::from_ascii(mario::core::kFallbackLevel, config, out_world, error)) {
    std::cerr << "Fallback level parse error: " << error << "\n";
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  Args args{};
  bool wants_help = false;
  if (!parse_args(argc, argv, args, wants_help)) {
    return wants_help ? 0 : 1;
  }

  const mario::core::Config config{};

  mario::core::Replay replay{};
  if (args.replay_path.has_value()) {
    std::string replay_contents;
    if (!read_file(*args.replay_path, replay_contents)) {
      std::cerr << "Failed to read replay: " << args.replay_path->string() << "\n";
      return 2;
    }
    std::string error;
    if (!mario::core::replay_from_jsonl(replay_contents, replay, error)) {
      std::cerr << "Failed to parse replay: " << error << "\n";
      return 2;
    }
    if (!replay.level.empty()) {
      args.level = replay.level;
    }
  }

  const fs::path level_path = args.assets_dir / fs::path(args.level);
  mario::core::World world{};
  if (!load_world_or_fallback(config, level_path, world)) {
    return 2;
  }

  mario::core::GameState state = mario::core::make_new_game(std::move(world), config);

  std::vector<mario::core::StepInput> used_inputs;
  const std::size_t default_headless_ticks = 600;
  const std::size_t replay_frames = replay.inputs.size();
  const std::size_t ticks_to_run = [&] {
    if (args.replay_path.has_value()) {
      return static_cast<std::size_t>(args.ticks.value_or(static_cast<std::int64_t>(replay_frames)));
    }
    return static_cast<std::size_t>(args.ticks.value_or(static_cast<std::int64_t>(default_headless_ticks)));
  }();

  used_inputs.reserve(ticks_to_run);

  for (std::size_t i = 0; i < ticks_to_run; ++i) {
    mario::core::StepInput input{};
    if (args.replay_path.has_value() && i < replay.inputs.size()) {
      input = replay.inputs[i];
    }
    used_inputs.push_back(input);
    mario::core::step(state, input);
  }

  const std::uint64_t hash = mario::core::hash_state(state);
  std::cout << "hash=0x" << std::hex << hash << std::dec << " ticks=" << state.tick << "\n";

  if (args.expect_hash.has_value() && hash != *args.expect_hash) {
    std::cerr << "Expected hash 0x" << std::hex << *args.expect_hash << " but got 0x" << hash
              << std::dec << "\n";
    return 3;
  }

  if (args.record_path.has_value()) {
    mario::core::Replay out_replay{};
    out_replay.version = 1;
    out_replay.level = args.level;
    out_replay.inputs = std::move(used_inputs);

    const std::string jsonl = mario::core::replay_to_jsonl(out_replay);
    if (!write_file(*args.record_path, jsonl)) {
      std::cerr << "Failed to write replay: " << args.record_path->string() << "\n";
      return 2;
    }
  }

  return 0;
}
