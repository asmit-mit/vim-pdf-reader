#pragma once
#include "core/cmd_loader.h"
#include "utils/trie.h"
#include <string>
#include <vector>

namespace core {

struct CmdAutocompleteItem {
  std::u32string name;
  std::u32string cmd;
  std::string description;
};

class CmdAutocomplete {
public:
  explicit CmdAutocomplete(const CmdLoader &cmd_loader);
  const std::vector<CmdAutocompleteItem> &complete(const std::string &prefix);

private:
  void tokenize(const std::string &input, std::vector<std::string> &argv);
  void push(const Cmd *cmd, const std::string &full_cmd);

private:
  utils::Trie trie_;
  std::vector<CmdAutocompleteItem> results_;
  const CmdLoader &cmd_loader_;
};

} // namespace core
