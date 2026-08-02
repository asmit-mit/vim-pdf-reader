#pragma once

#include <deque>
#include <string>
#include <vector>

namespace core {

class CmdHistory {
public:
  CmdHistory();

  void reset();
  void clearHistory();
  void clearRecentfiles();

  void add(const std::string &cmd);
  void addFileHistory(const std::string &path);

  std::string getNext();
  std::string getPrevious();

  std::vector<std::string> recentFiles() const;

private:
  void saveHistory();
  void saveRecentFiles();

private:
  std::deque<std::string> history_;
  std::deque<std::string> recent_files_;

  static constexpr std::size_t max_size_ = 100;
  std::size_t curr_idx_ = 0;

  std::string state_dir_;
  std::string history_file_;
  std::string recent_files_file_;
};

} // namespace core
