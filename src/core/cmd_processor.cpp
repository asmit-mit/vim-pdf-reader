#include <sstream>
#include <stdexcept>
#include <vector>

#include "core/cmd_processor.h"
#include "utils/utils.h"

namespace core {

CmdProcessor::CmdProcessor(
    EventBus &event_bus,
    HistoryManager &cmd_history,
    HistoryManager &search_history,
    HistoryManager &file_history
)
    : cmd_history_(cmd_history), search_history_(search_history), file_history_(file_history),
      event_bus_(event_bus) {
  commands_["open"] = {2, "Open document with absolute path"};
  commands_["reload"] = {1, "Reload current document"};
  commands_["close"] = {1, "Close current document"};
  commands_["clear"] = {2, "Clear history or recent files"};
  commands_["quit"] = {1, "Quit app"};
  commands_["bdelete"] = {2, "Delete bookmark"};
  commands_["bmark"] = {2, "Set bookmark"};
  commands_["blist"] = {1, "List bookmarks"};
  commands_["bjump"] = {2, "Jump to bookmark"};

  cmd_names_ = {"open", "reload", "close", "clear", "quit", "bdelete", "bmark", "blist", "bjump"};

  for (const auto &[command, _] : commands_)
    autocomplete_.insert(command);
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

  if (argv[0] == "open") {
    try {
      event_bus_.emit("cmd_processor.open_document", argv[1]);
      file_history_.add(argv[1]);
    } catch (const std::runtime_error &e) {
      throw e;
    }
  }
  if (argv[0] == "clear") {
    if (argv[1] == "files")
      file_history_.clear();
    if (argv[1] == "search")
      search_history_.clear();
    if (argv[1] == "history")
      file_history_.clear();
  }
  if (argv[0] == "close")
    event_bus_.emit("cmd_processor.close_document", true);
  if (argv[0] == "reload")
    event_bus_.emit("cmd_processor.reload_document", true);
  if (argv[0] == "quit")
    event_bus_.emit("cmd_processor.quit", true);
}

std::vector<std::pair<std::string, std::string>> CmdProcessor::complete(const std::string &input) {
  std::istringstream iss(input);
  std::vector<std::string> argv;
  std::string arg;
  while (iss >> arg)
    argv.push_back(arg);

  if (argv.empty()) {
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto &cmd : cmd_names_)
      result.emplace_back(cmd, commands_[cmd].second);
    return result;
  }

  if (argv.size() == 1 && input.back() != ' ') {
    const std::string prefix = argv.empty() ? "" : argv[0];
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto &cmd : autocomplete_.complete(prefix))
      result.emplace_back(cmd, commands_[cmd].second);
    return result;
  }

  const std::string &cmd = argv[0];

  if (cmd == "open") {
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto &file : file_history_.getAllUnique())
      result.emplace_back("open " + file, "");
    return result;
  }

  if (cmd == "clear") {
    std::vector<std::pair<std::string, std::string>> result;
    result.emplace_back("clear files", "Clear recent files");
    result.emplace_back("clear search", "Clear search history");
    result.emplace_back("clear history", "Clear command history");
    return result;
  }

  return {};
}

} // namespace core
