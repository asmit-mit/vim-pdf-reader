#include <utils/trie.h>

namespace utils {

Trie::Trie() {
  head_ = new TrieNode();
}

void Trie::insert(const std::string &word) {
  TrieNode *node = head_;

  for (char c : word) {
    int idx = tolower(c) - 'a';
    if (!node->children[idx])
      node->children[idx] = new TrieNode();

    node = node->children[idx];
  }

  node->is_terminal = true;
}

std::vector<std::string> Trie::complete(const std::string &prefix) const {
  TrieNode *node = head_;

  for (char c : prefix) {
    int idx = tolower(c) - 'a';
    if (!node->children[idx])
      return {};
    node = node->children[idx];
  }

  std::vector<std::string> matches;
  std::string partial = prefix;
  backtrack(node, partial, matches);

  return matches;
}

void Trie::backtrack(TrieNode *node, std::string &curr, std::vector<std::string> &matches) const {
  if (node->is_terminal)
    matches.push_back(curr);

  for (int i = 0; i < 26; i++) {
    if (!node->children[i])
      continue;
    curr.push_back('a' + i);
    backtrack(node->children[i], curr, matches);
    curr.pop_back();
  }
}

} // namespace utils
