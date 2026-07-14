#include <sstream>
#include <vector>

#include "core/cmd_processor.h"

namespace core {

CmdProcessor::CmdProcessor(core::EventBus &event_bus) : event_bus_(event_bus) {
  commands_["open"] = 2;
  commands_["close"] = 1;
  commands_["quit"] = 1;
}

void CmdProcessor::runCommand(const std::string &cmd) {
  std::istringstream iss(cmd);

  std::vector<std::string> argv;
  std::string arg;

  while (iss >> arg)
    argv.push_back(arg);

  if (!commands_.contains(argv[0])) {
    event_bus_.emit("status.msg", std::string("Not a valid command."));
    return;
  }

  unsigned long required_arg_count = commands_[argv[0]];
  if (argv.size() != required_arg_count) {
    event_bus_
        .emit("status.msg", "Provided " + std::to_string(argv.size() - 1) + " argument(s), but required " + std::to_string(required_arg_count - 1));
    return;
  }

  if (argv[0] == "open")
    event_bus_.emit("cmd_processor.open_document", argv[1]);
}

} // namespace core
