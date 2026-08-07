#include "core/history_manager.h"

#include <fstream>

namespace core {

HistoryManager::HistoryManager() {}

void HistoryManager::setPath(const std::string &path) {
  history_file_ = path;

  std::ifstream file(history_file_);

  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty()) {
      history_.push_back(line);
      history_set_.insert(line);
    }
  }

  reset();
  path_set_ = true;
}

void HistoryManager::setSaveUnique(bool unique) {
  save_unique_ = unique;
}

void HistoryManager::clear() {
  history_.clear();
  if (save_unique_)
    history_set_.clear();
  reset();
  save();
}

void HistoryManager::reset() {
  curr_idx_ = history_.size();
}

void HistoryManager::add(const std::string &cmd) {
  if (cmd.empty())
    return;

  if (save_unique_) {
    if (history_set_.count(cmd))
      return;
    history_set_.insert(cmd);
  }

  history_.push_back(cmd);
  if (history_.size() > max_size_) {
    history_set_.erase(history_.front());
    history_.pop_front();
  }
  reset();
  save();
}

std::string HistoryManager::getNext() {
  if (history_.empty())
    return "";
  if (curr_idx_ < history_.size())
    ++curr_idx_;
  if (curr_idx_ == history_.size())
    return "";
  return history_[curr_idx_];
}

std::string HistoryManager::getPrevious() {
  if (history_.empty())
    return "";
  if (curr_idx_ > 0)
    --curr_idx_;
  return history_[curr_idx_];
}

std::vector<std::string> HistoryManager::getAll() const {
  return {history_.begin(), history_.end()};
}

std::vector<std::string> HistoryManager::getAllUnique() const {
  return {history_set_.begin(), history_set_.end()};
}

void HistoryManager::save() {
  if (!path_set_)
    return;

  std::ofstream file(history_file_, std::ios::trunc);
  if (!file.is_open())
    return;
  for (const auto &cmd : history_)
    file << cmd << '\n';
}

} // namespace core
