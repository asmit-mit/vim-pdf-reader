#include "core/cmd_autocomplete.h"

#include <sstream>
#include <stdexcept>
#include <utf8.h>

#include "utils/utils.h"

namespace core {

CmdAutocomplete::CmdAutocomplete(const CmdLoader &cmd_loader) : cmd_loader_(cmd_loader) {
  for (const Cmd *cmd : cmd_loader_.getRootCmds())
    trie_.insert(cmd->name);
}

std::vector<CmdAutocompleteItem> CmdAutocomplete::complete(const std::string &prefix) {
  std::vector<CmdAutocompleteItem> res;
  std::vector<std::string> argv;
  tokenize(prefix, argv);

  if (argv.empty()) {
    for (const Cmd *cmd : cmd_loader_.getRootCmds())
      push(cmd, cmd->name + " ", res);
    return res;
  }

  if (argv.size() == 1 && !prefix.ends_with(' ')) {
    for (const std::string &name : trie_.complete(argv[0]))
      if (const Cmd *cmd = cmd_loader_.find(name))
        push(cmd, cmd->name + " ", res);
    return res;
  }

  std::vector<std::string> confirmed;
  std::string typing;

  if (prefix.ends_with(' ')) {
    confirmed = argv;
    typing = "";
  } else {
    confirmed = std::vector<std::string>(argv.begin(), argv.end() - 1);
    typing = argv.back();
  }

  std::string scoped_key = confirmed[0];
  for (size_t i = 1; i < confirmed.size(); ++i)
    scoped_key += "." + confirmed[i];

  const Cmd *cmd = cmd_loader_.find(scoped_key);
  if (!cmd)
    return res;

  switch (static_cast<CmdAutocompleteType>(cmd->cmp_type)) {
  case CmdAutocompleteType::Subcommand:
    getSubcmdCmp(scoped_key, typing, res);
    break;
  case CmdAutocompleteType::Files:
    getFileCmp(scoped_key, typing, res);
    break;
  default:
    throw std::runtime_error("Not valid completion type");
  }

  return res;
}

void CmdAutocomplete::tokenize(const std::string &input, std::vector<std::string> &argv) {
  std::istringstream iss(input);
  std::string token;
  while (iss >> token)
    argv.push_back(token);
}

void CmdAutocomplete::push(
    const Cmd *cmd, const std::string &full_cmd, std::vector<CmdAutocompleteItem> &results
) {
  results.push_back({
      cmd->name,
      full_cmd,
      cmd->description,
  });
}

void CmdAutocomplete::getSubcmdCmp(
    const std::string &scoped_key,
    const std::string &typing,
    std::vector<CmdAutocompleteItem> &results
) {
  for (const Cmd *child : cmd_loader_.childrenOf(scoped_key)) {
    if (child->name.starts_with(typing)) {
      std::string full = scoped_key + " " + child->name;
      std::replace(full.begin(), full.end(), '.', ' ');
      push(child, full + " ", results);
    }
  }
}

void CmdAutocomplete::getFileCmp(
    const std::string &scoped_key,
    const std::string &typing,
    std::vector<CmdAutocompleteItem> &results
) {
  std::string resolved = utils::resolvePath(typing);
  std::string top_cmd = scoped_key;
  std::replace(top_cmd.begin(), top_cmd.end(), '.', ' ');

  std::string dir;
  std::string prefix;

  auto slash = resolved.rfind('/');
  if (slash == std::string::npos) {
    dir = ".";
    prefix = resolved;
  } else {
    dir = resolved.substr(0, slash + 1);
    prefix = resolved.substr(slash + 1);
  }

  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    std::string name = entry.path().filename().string();
    if (name.starts_with(prefix)) {
      if (entry.is_directory()) {
        std::string file = dir + name + '/';
        results.push_back({file, top_cmd + " " + file, ""});
      } else if (entry.path().extension() == ".pdf") {
        std::string file = dir + name;
        results.push_back({file, top_cmd + " " + file, ""});
      }
    }
  }
}

} // namespace core
