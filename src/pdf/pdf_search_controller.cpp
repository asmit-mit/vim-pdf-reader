#include "pdf/pdf_search_controller.h"

namespace pdf {

PDFSearchController::PDFSearchController(std::vector<ui::PageView> &pages) : pages_(pages) {
  search_pos_dirty_ = false;
  show_search_result_boxes_ = false;

  curr_search_result_ = 0;
  local_search_result_ = 0;
  total_search_results_ = 0;
}

std::size_t PDFSearchController::search(
    const std::u32string &text, std::size_t anchor, pdf::PDFDocument &document
) {
  if (!document.isAllContentLoaded())
    document.loadAllContent();

  pages_with_search_results_.clear();
  int closest_page = -1;
  int closest_distance = std::numeric_limits<int>::max();

  std::size_t global_start = 0;
  for (std::size_t i = 0; i < document.pageCount(); i++) {
    pages_[i].clearSearchResults();
    ui::PDFSearchResult res;
    res.start = global_start;
    res.local_rects = document.getPage(i).searchText(text);
    global_start += res.local_rects.size();

    if (!res.local_rects.empty()) {
      pages_with_search_results_.push_back(i);
      int distance = std::abs(static_cast<int>(i) - static_cast<int>(anchor));
      if (distance < closest_distance) {
        closest_distance = distance;
        closest_page = static_cast<int>(i);
      }
    }

    pages_[i].setSearchResults(std::move(res));
  }

  if (closest_page == -1) {
    curr_search_result_ = 0;
    local_search_result_ = 0;
    total_search_results_ = 0;

    search_pos_dirty_ = false;
    show_search_result_boxes_ = false;

    return closest_page;
  }

  curr_search_result_ = pages_[closest_page].getSearchResults().start;
  local_search_result_ = 0;
  total_search_results_ = global_start;
  pages_[closest_page].setSelectedSearchResult(local_search_result_);

  search_pos_dirty_ = true;
  show_search_result_boxes_ = true;

  return closest_page;
}

std::size_t PDFSearchController::getCurrSearchResult() const {
  return curr_search_result_;
}

std::size_t PDFSearchController::getLocalSearchResult() const {
  return local_search_result_;
}

std::size_t PDFSearchController::getTotalSearchResult() const {
  return total_search_results_;
}

bool PDFSearchController::isShowingSearchResult() const {
  return show_search_result_boxes_;
}

bool PDFSearchController::isSearchPosDirty() const {
  return search_pos_dirty_;
}

void PDFSearchController::clearSearchPosDirty() {
  search_pos_dirty_ = false;
}

void PDFSearchController::hideSearchResults() {
  show_search_result_boxes_ = false;
}

void PDFSearchController::reset() {
  curr_search_result_ = 0;
  total_search_results_ = 0;
  hideSearchResults();
}

int PDFSearchController::goNext(std::size_t anchor) {
  if (total_search_results_ == 0)
    return -1;

  pages_[anchor].setSelectedSearchResult(-1);

  if (local_search_result_ + 1 < pages_[anchor].getSearchResults().local_rects.size()) {
    curr_search_result_++;
    local_search_result_++;
    pages_[anchor].setSelectedSearchResult(local_search_result_);

    search_pos_dirty_ = true;
    show_search_result_boxes_ = true;
    return -1;
  }

  auto it = std::
      upper_bound(pages_with_search_results_.begin(), pages_with_search_results_.end(), static_cast<int>(anchor));
  if (it == pages_with_search_results_.end())
    it = pages_with_search_results_.begin();

  curr_search_result_ = pages_[*it].getSearchResults().start;
  local_search_result_ = 0;

  pages_[anchor].setSelectedSearchResult(local_search_result_);
  search_pos_dirty_ = true;
  show_search_result_boxes_ = true;

  return *it;
}

int PDFSearchController::goPrev(std::size_t anchor) {
  if (total_search_results_ == 0)
    return -1;

  pages_[anchor].setSelectedSearchResult(-1);

  if (local_search_result_ > 0) {
    curr_search_result_--;
    local_search_result_--;
    pages_[anchor].setSelectedSearchResult(local_search_result_);

    search_pos_dirty_ = true;
    show_search_result_boxes_ = true;
    return -1;
  }

  auto it = std::
      lower_bound(pages_with_search_results_.begin(), pages_with_search_results_.end(), static_cast<int>(anchor));
  if (it == pages_with_search_results_.begin())
    it = pages_with_search_results_.end();
  --it;

  const std::size_t last_local = pages_[*it].getSearchResults().local_rects.size() - 1;
  curr_search_result_ = pages_[*it].getSearchResults().start + last_local;
  local_search_result_ = last_local;
  pages_[anchor].setSelectedSearchResult(local_search_result_);

  search_pos_dirty_ = true;
  show_search_result_boxes_ = true;

  return *it;
}

} // namespace pdf
