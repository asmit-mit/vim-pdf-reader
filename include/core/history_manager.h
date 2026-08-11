#pragma once

#include <deque>
#include <string>
#include <unordered_set>
#include <vector>

namespace core {

class HistoryManager {
public:
  HistoryManager();

  void setSaveUnique(bool unique);
  void setPath(const std::string &path);
  void reset();
  void clear();

  void add(const std::u32string &cmd);

  std::u32string getNext();
  std::u32string getPrevious();

  std::vector<std::u32string> getAll() const;
  std::vector<std::u32string> getAllUnique() const;

private:
  void save();

private:
  std::deque<std::u32string> history_;
  std::unordered_set<std::u32string> history_set_;

  static constexpr std::size_t max_size_ = 100;
  std::size_t curr_idx_ = 0;

  std::string history_file_;
  bool save_unique_ = false;
  bool path_set_ = false;
};

} // namespace core
