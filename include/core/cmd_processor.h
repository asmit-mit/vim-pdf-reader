#pragma once

#include <unordered_map>

#include "core/history_saver.h"
#include "core/event_bus.h"
#include "utils/trie.h"

namespace core {

class CmdProcessor {
public:
  CmdProcessor(EventBus &event_bus, HistorySaver &cmd_history, HistorySaver &search_history, HistorySaver &file_history);

  void runCommand(const std::string &cmd);
  std::vector<std::pair<std::string, std::string>> complete(const std::string &prefix);

private:
  HistorySaver &cmd_history_;
  HistorySaver &search_history_;
  HistorySaver &file_history_;

  core::EventBus &event_bus_;
  utils::Trie autocomplete_;

  std::vector<std::string> cmd_names_;
  std::unordered_map<std::string, std::pair<int, std::string>> commands_;
};

} // namespace core
