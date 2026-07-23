#pragma once

#include <deque>
#include <string>

namespace core {

class CmdHistory {
public:
  CmdHistory();

  void reset();
  void add(const std::string &cmd);
  std::string getNext();
  std::string getPrevious();

private:
  void save();

private:
  std::deque<std::string> history_;
  static constexpr std::size_t max_size_ = 100;
  std::size_t curr_idx_;

  static constexpr char history_file_path_[] = "/home/asmitpaul/.vim-reader-cmd-history";
};

} // namespace core
