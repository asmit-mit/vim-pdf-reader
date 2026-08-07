#pragma once

#include <unordered_map>

#include "core/history_manager.h"
#include "core/event_bus.h"
#include "utils/trie.h"

namespace core {

class CmdProcessor {
public:
  CmdProcessor(EventBus &event_bus, HistoryManager &cmd_history, HistoryManager &search_history, HistoryManager &file_history);

  void runCommand(const std::string &cmd);
  std::vector<std::pair<std::string, std::string>> complete(const std::string &prefix);

private:
  HistoryManager &cmd_history_;
  HistoryManager &search_history_;
  HistoryManager &file_history_;

  core::EventBus &event_bus_;
  utils::Trie autocomplete_;

  std::vector<std::string> cmd_names_;
  std::unordered_map<std::string, std::pair<int, std::string>> commands_;
};

} // namespace core
