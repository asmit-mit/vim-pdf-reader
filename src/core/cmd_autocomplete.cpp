#include "core/cmd_autocomplete.h"

#include <sstream>
#include <utf8.h>

namespace core {

CmdAutocomplete::CmdAutocomplete(const CmdLoader &cmd_loader) : cmd_loader_(cmd_loader) {
  for (const Cmd *cmd : cmd_loader_.getRootCmds())
    trie_.insert(cmd->name);
}

void CmdAutocomplete::push(const Cmd *cmd, const std::string &full_cmd) {
  results_.push_back({
      utf8::utf8to32(cmd->name),
      utf8::utf8to32(full_cmd),
      cmd->description,
  });
}

const std::vector<CmdAutocompleteItem> &CmdAutocomplete::complete(const std::string &prefix) {
  results_.clear();

  std::vector<std::string> argv;
  tokenize(prefix, argv);

  if (argv.empty()) {
    for (const Cmd *cmd : cmd_loader_.getRootCmds())
      push(cmd, cmd->name);
    return results_;
  }

  if (argv.size() == 1 && !prefix.ends_with(' ')) {
    for (const std::string &name : trie_.complete(argv[0]))
      if (const Cmd *cmd = cmd_loader_.find(name))
        push(cmd, cmd->name);
    return results_;
  }

  const std::string &root = argv[0];
  const std::string child_prefix = (argv.size() >= 2) ? argv[1] : "";

  for (const Cmd *child : cmd_loader_.childrenOf(root)) {
    if (child->name.starts_with(child_prefix)) {
      push(child, root + " " + child->name);
    }
  }

  return results_;
}

void CmdAutocomplete::tokenize(const std::string &input, std::vector<std::string> &argv) {
  std::istringstream iss(input);
  std::string token;
  while (iss >> token)
    argv.push_back(token);
}

} // namespace core
