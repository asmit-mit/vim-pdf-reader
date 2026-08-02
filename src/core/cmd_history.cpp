#include "core/cmd_history.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace core {

CmdHistory::CmdHistory() {
  const char *home = std::getenv("HOME");
  if (!home)
    home = ".";

  state_dir_ = std::string(home) + "/.local/state/vim-pdf-reader";
  history_file_ = state_dir_ + "/history";
  recent_files_file_ = state_dir_ + "/recent_files";

  fs::create_directories(state_dir_);

  {
    std::ifstream file(history_file_);

    std::string line;
    while (std::getline(file, line)) {
      if (!line.empty())
        history_.push_back(line);
    }
  }

  {
    std::ifstream file(recent_files_file_);

    std::string line;
    while (std::getline(file, line)) {
      if (!line.empty())
        recent_files_.push_back(line);
    }
  }

  reset();
}

void CmdHistory::clearHistory() {
  history_.clear();
  reset();
  saveHistory();
}

void CmdHistory::clearRecentfiles() {
  recent_files_.clear();
  saveRecentFiles();
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
  saveHistory();
}

void CmdHistory::addFileHistory(const std::string &path) {
  if (path.empty())
    return;

  auto it = std::find(recent_files_.begin(), recent_files_.end(), path);
  if (it != recent_files_.end())
    recent_files_.erase(it);

  recent_files_.push_front(path);

  if (recent_files_.size() > max_size_)
    recent_files_.pop_back();

  saveRecentFiles();
}

std::string CmdHistory::getNext() {
  if (history_.empty())
    return "";

  if (curr_idx_ < history_.size())
    ++curr_idx_;

  if (curr_idx_ == history_.size())
    return "";

  return history_[curr_idx_];
}

std::string CmdHistory::getPrevious() {
  if (history_.empty())
    return "";

  if (curr_idx_ > 0)
    --curr_idx_;

  return history_[curr_idx_];
}

std::vector<std::string> CmdHistory::recentFiles() const {
  return {recent_files_.begin(), recent_files_.end()};
}

void CmdHistory::saveHistory() {
  std::ofstream file(history_file_, std::ios::trunc);

  if (!file.is_open())
    return;

  for (const auto &cmd : history_)
    file << cmd << '\n';
}

void CmdHistory::saveRecentFiles() {
  std::ofstream file(recent_files_file_, std::ios::trunc);

  if (!file.is_open())
    return;

  for (const auto &path : recent_files_)
    file << path << '\n';
}

} // namespace core
