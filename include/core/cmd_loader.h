#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <simdjson.h>

namespace core {

struct Cmd {
  std::string name;
  std::optional<int> args;
  std::optional<std::string> event;
  std::string description;
  std::vector<size_t> children;
};

class CmdLoader {
public:
  explicit CmdLoader(const std::filesystem::path path);

  std::vector<const Cmd *> getRootCmds() const;
  const Cmd *find(const std::string &scoped_key) const;
  std::vector<const Cmd *> childrenOf(const std::string &scoped_key) const;

private:
  size_t parseCmd(simdjson::dom::element obj, const std::string &parent_key, int depth);

private:
  std::vector<Cmd> commands_;
  std::vector<std::size_t> roots_;
  std::unordered_map<std::string, std::size_t> index_;
};

} // namespace core
