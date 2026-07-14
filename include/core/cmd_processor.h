#pragma once

#include <unordered_map>

#include "core/event_bus.h"

namespace core {

class CmdProcessor {
public:
  CmdProcessor(core::EventBus &event_bus);

  void runCommand(const std::string &cmd);

private:
  core::EventBus &event_bus_;

  std::unordered_map<std::string, int> commands_;
};

} // namespace core
