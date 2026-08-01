#include <sstream>
#include <stdexcept>
#include <vector>

#include "core/cmd_processor.h"
#include "utils/utils.h"

namespace core {

CmdProcessor::CmdProcessor(core::EventBus &event_bus) : event_bus_(event_bus) {
  commands_["open"] = {2, "Open document with absolute path"};
  commands_["reload"] = {1, "Reload current Document"};
  commands_["close"] = {1, "Close current document"};
  commands_["quit"] = {1, "Quit app"};
  // commands_["blist"] = {1, "Quit app"};
  // commands_["badd"] = {1, "Quit app"};
  // commands_["bdel"] = {1, "Quit app"};

  for (const auto &[command, _] : commands_)
    autocomplete_.insert(command);

  event_bus_.subscribe<const char *>("cmdline.msg", [](const char *msg) {
    throw std::runtime_error(msg);
  });
}

void CmdProcessor::runCommand(const std::string &cmd) {
  if (cmd.empty())
    return;

  std::istringstream iss(cmd);

  std::vector<std::string> argv;
  std::string arg;

  while (iss >> arg)
    argv.push_back(arg);

  if (utils::isNumber(argv[0])) {
    event_bus_.emit("cmd_processor.switch_page", std::stoi(argv[0]));
    return;
  }

  if (!commands_.contains(argv[0]))
    throw std::runtime_error("Not a valid command.");

  unsigned long required_arg_count = commands_[argv[0]].first;
  if (argv.size() != required_arg_count)
    throw std::runtime_error(
        "Provided " + std::to_string(argv.size() - 1) + " argument(s), but required " +
        std::to_string(required_arg_count - 1)
    );

  if (argv[0] == "open")
    event_bus_.emit("cmd_processor.open_document", argv[1]);

  if (argv[0] == "close")
    event_bus_.emit("cmd_processor.close_document", true);

  if (argv[0] == "reload")
    event_bus_.emit("cmd_processor.reload_document", true);

  if (argv[0] == "quit")
    event_bus_.emit("cmd_processor.quit", true);
}

std::vector<std::pair<std::string, std::string>> CmdProcessor::complete(const std::string &prefix) {
  if (prefix.size() == 0) {
    const auto &cmd_list = autocomplete_.complete("");
    std::vector<std::pair<std::string, std::string>> completions;
    for (const auto &cmd : cmd_list)
      completions.emplace_back(cmd, commands_[cmd].second);
    return completions;
  }

  std::istringstream iss(prefix);

  std::vector<std::string> argv;
  std::string arg;

  while (iss >> arg)
    argv.push_back(arg);

  if (argv.size() > 1)
    return {};

  const auto &cmd_list = autocomplete_.complete(argv[0]);
  std::vector<std::pair<std::string, std::string>> completions;
  for (const auto &cmd : cmd_list)
    completions.emplace_back(cmd, commands_[cmd].second);

  return completions;
}

} // namespace core
