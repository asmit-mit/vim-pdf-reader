#pragma once

#include <unordered_map>

#include "core/event_bus.h"
#include "utils/trie.h"

namespace core {

class CmdProcessor {
public:
  CmdProcessor(core::EventBus &event_bus);

  void runCommand(const std::string &cmd);
  std::vector<std::string> complete(const std::string &prefix);

private:
  core::EventBus &event_bus_;
  utils::Trie autocomplete_;

  std::unordered_map<std::string, std::pair<int, std::string>> commands_;
};

} // namespace core
