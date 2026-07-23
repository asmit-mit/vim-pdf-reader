#include <fstream>

#include "core/cmd_history.h"

namespace core {

CmdHistory::CmdHistory() {
  std::ifstream file(history_file_path_);

  if (!file.is_open())
    return;

  std::string line;
  while (std::getline(file, line))
    add(line);
}

void CmdHistory::reset() {
  curr_idx_ = history_.size();
}

void CmdHistory::add(const std::string &cmd) {
  if (cmd.empty())
    return;

  history_.push_back(cmd);

  if (history_.size() > max_size_)
    history_.pop_front();

  reset();
  save();
}

std::string CmdHistory::getNext() {
  if (history_.empty())
    return "";

  if (curr_idx_ < history_.size())
    curr_idx_++;

  if (curr_idx_ == history_.size())
    return "";

  return history_[curr_idx_];
}

std::string CmdHistory::getPrevious() {
  if (history_.empty())
    return "";

  if (curr_idx_ > 0)
    curr_idx_--;

  return history_[curr_idx_];
}

void CmdHistory::save() {
  std::ofstream file(history_file_path_, std::ios::trunc);

  if (!file.is_open())
    return;

  for (const auto &cmd : history_)
    file << cmd << '\n';
}

} // namespace core
