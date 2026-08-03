#pragma once

#include <deque>
#include <string>
#include <vector>

namespace core {

class HistorySaver {
public:
  HistorySaver();

  void setPath(const std::string &path);
  void reset();
  void clear();

  void add(const std::string &cmd);

  std::string getNext();
  std::string getPrevious();
  std::vector<std::string> getAll() const;

private:
  void save();

private:
  std::deque<std::string> history_;

  static constexpr std::size_t max_size_ = 100;
  std::size_t curr_idx_ = 0;

  std::string history_file_;
};

} // namespace core
