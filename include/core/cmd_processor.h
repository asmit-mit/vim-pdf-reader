#pragma once

#include <unordered_map>

#include "core/cmd_history.h"
#include "core/event_bus.h"
#include "utils/trie.h"

namespace core {

class CmdProcessor {
public:
  CmdProcessor(EventBus &event_bus, CmdHistory &history);

  void runCommand(const std::string &cmd);
  std::vector<std::pair<std::string, std::string>> complete(const std::string &prefix);

private:
  CmdHistory &history_;

  core::EventBus &event_bus_;
  utils::Trie autocomplete_;

  std::unordered_map<std::string, std::pair<int, std::string>> commands_;
};

} // namespace core
