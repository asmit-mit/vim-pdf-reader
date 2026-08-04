#pragma once

#include <deque>
#include <string>
#include <vector>
#include <unordered_set>

namespace core {

class HistorySaver {
public:
  HistorySaver();

  void setSaveUnique(bool unique);
  void setPath(const std::string &path);
  void reset();
  void clear();

  void add(const std::string &cmd);

  std::string getNext();
  std::string getPrevious();
  std::vector<std::string> getAllUnique() const;

private:
  void save();

private:
  std::deque<std::string> history_;
  std::unordered_set<std::string> history_set_;

  static constexpr std::size_t max_size_ = 100;
  std::size_t curr_idx_ = 0;

  std::string history_file_;
  bool save_unique_ = false;
};

} // namespace core
