#include "core/cmd_loader.h"

#include <stdexcept>

namespace core {

CmdLoader::CmdLoader(const std::filesystem::path path) {
  auto json = simdjson::padded_string::load(path.generic_string());
  if (json.error())
    throw std::runtime_error(simdjson::error_message(json.error()));

  simdjson::ondemand::parser parser;
  auto doc = parser.iterate(json);
  if (doc.error())
    throw std::runtime_error(simdjson::error_message(doc.error()));

  for (auto cmd : doc.get_array())
    parseCmd(cmd.get_object().value(), "", 0);
}

std::vector<const Cmd *> CmdLoader::getRootCmds() const {
  std::vector<const Cmd *> res;

  res.reserve(roots_.size());
  for (std::size_t idx : roots_)
    res.push_back(&commands_[idx]);

  return res;
}

const Cmd *CmdLoader::find(const std::string &scoped_key) const {
  auto it = index_.find(scoped_key);
  if (it == index_.end())
    return nullptr;

  return &commands_[it->second];
}

std::vector<const Cmd *> CmdLoader::childrenOf(const std::string &scoped_key) const {
  const Cmd *cmd = find(scoped_key);
  if (!cmd)
    return {};

  std::vector<const Cmd *> res;
  res.reserve(cmd->children.size());
  for (size_t idx : cmd->children)
    res.push_back(&commands_[idx]);

  return res;
}

std::size_t
CmdLoader::parseCmd(simdjson::ondemand::object obj, const std::string &parent_key, int depth) {
  Cmd cmd;

  cmd.name = std::string(obj["name"].get_string().value());
  cmd.description = std::string(obj["description"].get_string().value());

  int64_t args;
  if (obj["args"].get_int64().get(args) == simdjson::SUCCESS)
    cmd.args = static_cast<int32_t>(args);

  std::string event;
  if (obj["event"].get_string().get(event) == simdjson::SUCCESS)
    cmd.event = std::move(event);

  std::string scoped_key = parent_key.empty() ? cmd.name : parent_key + "." + cmd.name;

  size_t idx = commands_.size();
  if (depth == 0)
    roots_.push_back(idx);
  commands_.push_back({});

  for (auto child : obj["children"].get_array()) {
    size_t child_idx = parseCmd(child.get_object().value(), scoped_key, depth + 1);
    commands_[idx].children.push_back(child_idx);
  }

  commands_[idx] = std::move(cmd);
  index_[scoped_key] = idx;
  return idx;
}

} // namespace core
