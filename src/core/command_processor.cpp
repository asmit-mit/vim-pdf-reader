#include <sstream>
#include <vector>

#include "core/command_processor.h"

namespace core {

CommandProcessor::CommandProcessor(core::EventBus &event_bus) : event_bus_(event_bus) {
  commands_["open"] = 2;
  commands_["close"] = 1;
  commands_["quit"] = 1;

  event_bus_.subscribe<std::string>("cmdline.cmd", [this](const std::string &cmd) {
    runCommand(cmd);
  });
}

void CommandProcessor::runCommand(const std::string &cmd) {
  std::istringstream iss(cmd.substr(1));

  std::vector<std::string> argv;
  std::string arg;

  while (iss >> arg)
    argv.push_back(arg);

  if (!commands_.contains(argv[0])) {
    event_bus_.emit("status.msg", std::string("Not a valid command."));
    return;
  }

  int required_arg_count = commands_[argv[0]];
  if (argv.size() != required_arg_count) {
    event_bus_
        .emit("status.msg", "Provided " + std::to_string(argv.size() - 1) + " argument(s), but required " + std::to_string(required_arg_count - 1));
    return;
  }
}

} // namespace core
