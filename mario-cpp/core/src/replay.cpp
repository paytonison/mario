#include "mario/core/replay.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <string>
#include <string_view>

namespace mario::core {
namespace {

std::string_view trim(std::string_view s) {
  while (!s.empty()) {
    const unsigned char ch = static_cast<unsigned char>(s.front());
    if (std::isspace(ch) == 0) {
      break;
    }
    s.remove_prefix(1);
  }
  while (!s.empty()) {
    const unsigned char ch = static_cast<unsigned char>(s.back());
    if (std::isspace(ch) == 0) {
      break;
    }
    s.remove_suffix(1);
  }
  return s;
}

bool find_json_value(std::string_view line, std::string_view key, std::string_view& out_value) {
  const std::string_view needle_prefix = "\"";
  std::string needle;
  needle.reserve(key.size() + 2);
  needle.append(needle_prefix);
  needle.append(key);
  needle.push_back('"');

  const std::size_t key_pos = line.find(needle);
  if (key_pos == std::string_view::npos) {
    return false;
  }
  std::size_t pos = key_pos + needle.size();
  pos = line.find(':', pos);
  if (pos == std::string_view::npos) {
    return false;
  }
  pos += 1;
  while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])) != 0) {
    pos += 1;
  }
  if (pos >= line.size()) {
    return false;
  }

  std::size_t end = pos;
  if (line[pos] == '"') {
    end = line.find('"', pos + 1);
    if (end == std::string_view::npos) {
      return false;
    }
    out_value = line.substr(pos, (end - pos) + 1);
    return true;
  }

  while (end < line.size()) {
    const char ch = line[end];
    if ((ch >= '0' && ch <= '9') || ch == '-' || ch == '+') {
      end += 1;
      continue;
    }
    break;
  }
  if (end == pos) {
    return false;
  }
  out_value = line.substr(pos, end - pos);
  return true;
}

bool parse_json_int(std::string_view line, std::string_view key, std::int64_t& out) {
  std::string_view v;
  if (!find_json_value(line, key, v)) {
    return false;
  }
  v = trim(v);
  std::int64_t parsed = 0;
  const auto result = std::from_chars(v.data(), v.data() + v.size(), parsed);
  if (result.ec != std::errc{}) {
    return false;
  }
  out = parsed;
  return true;
}

bool parse_json_bool01(std::string_view line, std::string_view key, bool& out) {
  std::int64_t v = 0;
  if (!parse_json_int(line, key, v)) {
    return false;
  }
  out = (v != 0);
  return true;
}

bool parse_json_string(std::string_view line, std::string_view key, std::string& out) {
  std::string_view v;
  if (!find_json_value(line, key, v)) {
    return false;
  }
  v = trim(v);
  if (v.size() < 2 || v.front() != '"' || v.back() != '"') {
    return false;
  }
  out.assign(v.substr(1, v.size() - 2));
  return true;
}

}  // namespace

std::string replay_to_jsonl(const Replay& replay) {
  std::ostringstream oss;
  oss << "{\"version\":" << replay.version << ",\"level\":\"" << replay.level << "\"}\n";

  for (const StepInput& in : replay.inputs) {
    oss << "{\"l\":" << (in.left ? 1 : 0) << ",\"r\":" << (in.right ? 1 : 0)
        << ",\"jp\":" << (in.jump_pressed ? 1 : 0) << ",\"jr\":" << (in.jump_released ? 1 : 0)
        << ",\"start\":" << (in.start_pressed ? 1 : 0)
        << ",\"restart\":" << (in.restart_pressed ? 1 : 0)
        << ",\"quit\":" << (in.quit_pressed ? 1 : 0) << "}\n";
  }

  return oss.str();
}

bool replay_from_jsonl(std::string_view jsonl, Replay& out, std::string& error) {
  out = Replay{};
  error.clear();

  std::size_t line_no = 0;
  bool saw_any_frames = false;

  std::size_t pos = 0;
  while (pos <= jsonl.size()) {
    const std::size_t next_nl = jsonl.find('\n', pos);
    const std::size_t end = (next_nl == std::string_view::npos) ? jsonl.size() : next_nl;
    std::string_view line = trim(jsonl.substr(pos, end - pos));
    line_no += 1;

    if (!line.empty() && line.front() != '#') {
      std::int64_t version = 0;
      std::string level;
      if (!saw_any_frames && parse_json_int(line, "version", version) &&
          parse_json_string(line, "level", level)) {
        if (version <= 0 || version > 0xffff) {
          error = "Invalid replay version on line 1";
          return false;
        }
        out.version = static_cast<std::uint32_t>(version);
        out.level = std::move(level);
      } else {
        StepInput in{};
        bool ok = true;
        ok &= parse_json_bool01(line, "l", in.left);
        ok &= parse_json_bool01(line, "r", in.right);
        ok &= parse_json_bool01(line, "jp", in.jump_pressed);
        ok &= parse_json_bool01(line, "jr", in.jump_released);
        ok &= parse_json_bool01(line, "start", in.start_pressed);
        ok &= parse_json_bool01(line, "restart", in.restart_pressed);
        ok &= parse_json_bool01(line, "quit", in.quit_pressed);
        if (!ok) {
          error = "Replay parse error on line " + std::to_string(line_no);
          return false;
        }
        out.inputs.push_back(in);
        saw_any_frames = true;
      }
    }

    if (next_nl == std::string_view::npos) {
      break;
    }
    pos = next_nl + 1;
  }

  if (out.inputs.empty()) {
    error = "Replay has no input frames";
    return false;
  }

  return true;
}

}  // namespace mario::core

