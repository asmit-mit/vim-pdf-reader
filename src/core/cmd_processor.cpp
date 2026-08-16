#include <sstream>
#include <stdexcept>
#include <utf8.h>
#include <vector>

#include "core/cmd_processor.h"
#include "utils/utils.h"

namespace core {

CmdProcessor::CmdProcessor(
    EventBus &event_bus, const CmdLoader &cmd_loader, HistoryManager &cmd_history
)
    : cmd_loader_(cmd_loader), cmd_history_(cmd_history), event_bus_(event_bus) {}

void CmdProcessor::runCommand(const std::string &cmd) {
  if (cmd.empty())
    return;

  std::vector<std::string> argv;
  tokenize(cmd, argv);

  if (utils::isNumber(argv[0])) {
    event_bus_.emit("cmd.switch_page", std::stoi(argv[0]));
    return;
  }

  std::string arg = argv[0];
  const Cmd *cmd_ptr = cmd_loader_.find(arg);
  if (!cmd_ptr)
    throw std::runtime_error("Not a valid command: " + arg);

  if (cmd_ptr->args.has_value() && argv.size() != (size_t)cmd_ptr->args.value())
    throw std::runtime_error(
        "Provided " + std::to_string(argv.size() - 1) + " argument(s), but required " +
        std::to_string(cmd_ptr->args.value() - 1)
    );

  if (cmd_ptr->event.has_value()) {
    if (cmd_ptr->args.has_value() && cmd_ptr->args.value() == 1)
      event_bus_.emit(cmd_ptr->event.value(), true);
    else
      event_bus_.emit(cmd_ptr->event.value(), argv.size() > 1 ? argv[1] : "");
    return;
  }

  for (std::size_t i = 1; i < argv.size(); i++) {
    arg += "." + argv[i];
    cmd_ptr = cmd_loader_.find(arg);
    if (!cmd_ptr)
      throw std::runtime_error("Not a valid subcommand: " + arg);

    if (cmd_ptr->event.has_value()) {
      if ((i + 1) < (size_t)argv.size())
        event_bus_.emit(cmd_ptr->event.value(), argv[i + 1]);
      else
        event_bus_.emit(cmd_ptr->event.value(), true);
      return;
    }
  }

  throw std::runtime_error("Incomplete command: " + arg);
}

void CmdProcessor::tokenize(const std::string &cmd, std::vector<std::string> &argv) {
  std::istringstream iss(cmd);
  std::string token;
  while (iss >> token)
    argv.push_back(token);
}

} // namespace core
