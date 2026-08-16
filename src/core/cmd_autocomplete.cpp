#include "core/cmd_autocomplete.h"

#include <sstream>
#include <utf8.h>

namespace core {

CmdAutocomplete::CmdAutocomplete(const CmdLoader &cmd_loader) : cmd_loader_(cmd_loader) {
  for (const Cmd *cmd : cmd_loader_.getRootCmds())
    trie_.insert(cmd->name);
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

  for (const Cmd *child : cmd_loader_.childrenOf(scoped_key)) {
    if (child->name.starts_with(typing)) {
      std::string full = scoped_key + " " + child->name;
      std::replace(full.begin(), full.end(), '.', ' ');
      push(child, full + " ", res);
    }
  }

  return res;
}

void CmdAutocomplete::tokenize(const std::string &input, std::vector<std::string> &argv) {
  std::istringstream iss(input);
  std::string token;
  while (iss >> token)
    argv.push_back(token);
}

} // namespace core
