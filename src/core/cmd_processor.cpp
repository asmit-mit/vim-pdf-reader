#include <sstream>
#include <stdexcept>
#include <vector>

#include "core/cmd_processor.h"

namespace core {

CmdProcessor::CmdProcessor(core::EventBus &event_bus) : event_bus_(event_bus) {
  commands_["open"] = 2;
  commands_["close"] = 1;
  commands_["quit"] = 1;
}

void CmdProcessor::runCommand(const std::string &cmd) {
  if (cmd.empty())
    return;

  std::istringstream iss(cmd);

  std::vector<std::string> argv;
  std::string arg;

  while (iss >> arg)
    argv.push_back(arg);

  if (!commands_.contains(argv[0]))
    throw std::runtime_error("Not a valid command.");

  unsigned long required_arg_count = commands_[argv[0]];
  if (argv.size() != required_arg_count)
    throw std::runtime_error(
        "Provided " + std::to_string(argv.size() - 1) + " argument(s), but required " +
        std::to_string(required_arg_count - 1)
    );

  if (argv[0] == "open")
    event_bus_.emit("cmd_processor.open_document", argv[1]);
}

} // namespace core
