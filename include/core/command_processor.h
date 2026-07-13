#pragma once

#include <unordered_map>

#include "core/event_bus.h"

namespace core {

class CommandProcessor {
public:
  CommandProcessor(core::EventBus &event_bus);

private:
  void runCommand(const std::string &cmd);

private:
  core::EventBus &event_bus_;

  std::unordered_map<std::string, int> commands_;
};

} // namespace core
