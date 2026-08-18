#include "ui/page_view.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utf8.h>

#include "pdf/pdf_renderer.h"
#include "ui/pdf_view.h"
#include "ui/ui_elements.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

PDFView::PDFView(
    core::HistoryManager &file_history,
    pdf::PDFDocument &document,
    core::RenderScheduler &scheduler,
    core::EventBus &event_bus
)
    : event_bus_(event_bus), file_history_(file_history), document_(document),
      scheduler_(scheduler) {
  has_document_ = false;
  should_take_input_ = false;
  need_initial_pos_ = false;
  window_size_changed_ = false;
  pending_page_update_ = false;
  started_scrolling_ = false;
  page_positions_dirty_ = false;
  sync_state_dirty_ = false;
  search_pos_dirty_ = false;
  show_search_result_boxes_ = false;

  front_page_ = 0;
  anchor_page_ = 0;
  back_page_ = 0;

  curr_search_result_ = 0;
  local_search_result_ = 0;
  total_search_results_ = 0;

  anchor_page_pos_before_scroll_ = {0.f, 0.f};

  page_with_max_width_ = 0;

  event_bus_.subscribe<std::string>("cmd.open_document", [this](const std::string &filepath) {
    onOpenDocument(filepath);
  });

  event_bus_.subscribe<bool>("cmd.reload_document", [this](bool) {
    if (!has_document_) {
      event_bus_.emit("notification.msg", std::string("No document open to reload"));
      return;
    }

    onOpenDocument(filepath_);
  });

  event_bus_.subscribe<bool>("cmd.close_document", [this](bool) { onCloseDocument(); });

  event_bus_.subscribe<int>("cmd.switch_page", [this](int page_num) {
    onSwitchPage(page_num - 1);
  });

  event_bus_.subscribe<const std::u32string &>("cmd.search", [this](const std::u32string &text) {
    onSearchPage(text);
  });

  event_bus_.subscribe<ui::UIElements>("ui.focus", [this](ui::UIElements focus) {
    should_take_input_ = focus == ui::UIElements::PDFView;
  });
}

void PDFView::draw(sf::RenderTarget &window) const {
  if (!has_document_)
    return;

  if (need_initial_pos_)
    return;

  for (std::size_t i = front_page_; i <= back_page_; i++)
    pages_[i].draw(window);
}

void PDFView::update() {
  if (!has_document_)
    return;

  if (pending_page_update_ &&
      scale_rot_update_timer_.getElapsedTime().asMilliseconds() > scale_rot_debounce_ms_) {
    requestPage(anchor_page_, target_state_.zoom, target_state_.rotate);
    pending_page_update_ = false;
  }

  renderRequestedPages();
  setInitialPagePos();

  if (sync_state_dirty_) {
    syncWithTargetState();
    sync_state_dirty_ = false;
    page_positions_dirty_ = true;
  }

  if (page_positions_dirty_) {
    updatePagePositions();
    for (std::size_t i = front_page_; i <= back_page_; i++)
      pages_[i].syncPageShapePos();
    page_positions_dirty_ = false;
  }

  if (search_pos_dirty_ && pages_[anchor_page_].hasTexture()) {
    const sf::Vector2f window_center = {window_size_.x * 0.5f, window_size_.y * 0.5f};
    const sf::Vector2f pos = pages_[anchor_page_].getSearchResultPosition(local_search_result_);
    sf::Vector2f delta = window_center - pos;
    panCurrentPage(delta);
    updatePagePositions();
    for (std::size_t i = front_page_; i <= back_page_; i++)
      pages_[i].syncPageShapePos();
    page_positions_dirty_ = false;
    search_pos_dirty_ = false;
  }

  if (started_scrolling_)
    checkForAnchorPage();

  for (std::size_t i = front_page_; i <= back_page_; i++) {
    if (show_search_result_boxes_)
      pages_[i].showSearchResults();
    else
      pages_[i].hideSearchResults();
    pages_[i].update();
  }
}

void PDFView::handleEvent(const sf::Event &event) {
  if (!has_document_)
    return;

  const auto *key = event.getIf<sf::Event::KeyPressed>();
  if (key && should_take_input_) {
    if (key->code == sf::Keyboard::Key::U) {
      if (key->control)
        panCurrentPage({0.f, window_size_.y * 0.5f});
      else
        onSwitchPage(std::max(0, (int)anchor_page_ - 1));
    } else if (key->code == sf::Keyboard::Key::D) {
      if (key->control)
        panCurrentPage({0.f, -window_size_.y * 0.5f});
      else
        onSwitchPage(std::min(document_.pageCount() - 1, anchor_page_ + 1));
    } else if (key->code == sf::Keyboard::Key::Equal && key->control) {
      setZoom(target_state_.zoom + settings::delta_zoom_);
    } else if (key->code == sf::Keyboard::Key::Hyphen && key->control) {
      setZoom(target_state_.zoom - settings::delta_zoom_);
    } else if (key->code == sf::Keyboard::Key::H) {
      panCurrentPage({scroll_dist_, 0.f});
    } else if (key->code == sf::Keyboard::Key::L) {
      panCurrentPage({-scroll_dist_, 0.f});
    } else if (key->code == sf::Keyboard::Key::K) {
      panCurrentPage({0.f, scroll_dist_});
    } else if (key->code == sf::Keyboard::Key::J) {
      panCurrentPage({0.f, -scroll_dist_});
    } else if (key->code == sf::Keyboard::Key::N) {
      if (key->shift)
        goPrevPageWithResult();
      else
        goNextPageWithResult();
    } else if (key->code == sf::Keyboard::Key::R) {
      if (key->shift)
        setRotate((target_state_.rotate - 1) % 4);
      else
        setRotate((target_state_.rotate + 1) % 4);
    } else if (key->code == sf::Keyboard::Key::F) {
      pdf::PDFRenderKey base_key{anchor_page_, 1.f, target_state_.rotate};
      const auto page_dims = scheduler_.getPageSize(base_key);
      if (page_dims.x > 0 && page_dims.y > 0) {
        const float viewable_h = window_size_.y - utils::cmdline_height_;
        const float zoom_x = window_size_.x / static_cast<float>(page_dims.x);
        const float zoom_y = viewable_h / static_cast<float>(page_dims.y);
        setZoom(std::min(zoom_x, zoom_y));
      }
    } else if (key->code == sf::Keyboard::Key::W) {
      pdf::PDFRenderKey base_key{anchor_page_, 1.f, target_state_.rotate};
      const auto page_dims = scheduler_.getPageSize(base_key);
      if (page_dims.x > 0)
        setZoom(window_size_.x / static_cast<float>(page_dims.x));
    } else if (key->code == sf::Keyboard::Key::Escape) {
      curr_search_result_ = 0;
      total_search_results_ = 0;
      event_bus_
          .emit("statusbar.search_state", std::make_pair(curr_search_result_, total_search_results_));
      show_search_result_boxes_ = false;
    }
  }
}

void PDFView::onResize(const sf::Vector2f &size) {
  old_window_size_ = window_size_;
  window_size_ = size;
  window_size_changed_ = true;
  page_positions_dirty_ = true;
}

void PDFView::onOpenDocument(const std::string &filepath) {
  try {
    std::string absolute_path = utils::resolvePath(filepath);
    scheduler_.quiesce();
    document_.openDocument(absolute_path);
    filepath_ = filepath;
    scheduler_.clearCache();
    scheduler_.resume();

    has_document_ = true;
    resetView();
    onSwitchPage(anchor_page_);
    page_with_max_width_ = document_.pageWithMaxWidth();

    event_bus_.emit("statusbar.pdf_path", absolute_path);
    event_bus_.emit("statusbar.page_state", std::make_pair(anchor_page_ + 1, document_.pageCount()));
    event_bus_.emit("statusbar.page_zoom", target_state_.zoom);

    file_history_.add(absolute_path);
  } catch (const std::runtime_error &e) {
    onCloseDocument();
    event_bus_.emit("notification.msg", std::string(e.what()));
  }
}

void PDFView::onCloseDocument() {
  if (!has_document_)
    return;
  has_document_ = false;
  resetView();
  document_.closeDocument();
  event_bus_.emit("statusbar.pdf_path", std::string("[No document open]"));
  event_bus_.emit("statusbar.total_pages", static_cast<size_t>(0));
}

void PDFView::onSwitchPage(int page_idx) {
  if (!has_document_) {
    event_bus_.emit("notification.msg", std::string("No document currently open"));
    return;
  }

  if (page_idx < 0 || page_idx >= static_cast<int>(document_.pageCount())) {
    event_bus_.emit("notification.msg", std::string("Page number out of range"));
    return;
  }

  anchor_page_ = static_cast<std::size_t>(page_idx);
  requestPage(anchor_page_, target_state_.zoom, target_state_.rotate);
  event_bus_.emit("statusbar.page_state", std::make_pair(anchor_page_ + 1, document_.pageCount()));

  need_initial_pos_ = true;
  started_scrolling_ = false;
}

void PDFView::onSearchPage(const std::u32string &text) {
  if (!has_document_) {
    event_bus_.emit("notification.msg", std::string("No document open to search"));
    return;
  }
  if (!document_.isAllContentLoaded())
    document_.loadAllContent();

  pages_with_search_results_.clear();
  int closest_page = -1;
  int closest_distance = std::numeric_limits<int>::max();

  std::size_t global_start = 0;
  for (std::size_t i = 0; i < document_.pageCount(); i++) {
    pages_[i].clearSearchResults();
    PDFSearchResult res;
    res.start = global_start;
    res.local_rects = document_.getPage(i).searchText(text);
    global_start += res.local_rects.size();

    if (!res.local_rects.empty()) {
      pages_with_search_results_.push_back(i);
      int distance = std::abs(static_cast<int>(i) - static_cast<int>(anchor_page_));
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
    event_bus_
        .emit("statusbar.search_state", std::make_pair(curr_search_result_, total_search_results_));
    event_bus_.emit("notification.msg", std::string("No match found for: ") + utf8::utf32to8(text));
    show_search_result_boxes_ = false;
    return;
  }

  if (closest_page != static_cast<int>(anchor_page_))
    onSwitchPage(closest_page);

  curr_search_result_ = pages_[closest_page].getSearchResults().start;
  local_search_result_ = 0;
  total_search_results_ = global_start;
  pages_[closest_page].setSelectedSearchResult(local_search_result_);

  event_bus_
      .emit("statusbar.search_state", std::make_pair(curr_search_result_ + 1, total_search_results_));

  search_pos_dirty_ = true;
  show_search_result_boxes_ = true;
}

void PDFView::setInitialPagePos() {
  if (!need_initial_pos_)
    return;

  if (!pages_[anchor_page_].hasTexture())
    return;

  const auto size = scheduler_.getPageSize(pages_[anchor_page_].getKey());

  float x = window_size_.x * 0.5f;
  float y = size.y * 0.5f;
  if (size.y <= (window_size_.y - utils::cmdline_height_))
    y = (window_size_.y - utils::cmdline_height_) * 0.5;

  pages_[anchor_page_].setPosition({x, y});
  need_initial_pos_ = false;
  page_positions_dirty_ = true;
}

void PDFView::syncWithTargetState() {
  for (std::size_t i = front_page_; i <= back_page_; ++i) {
    pdf::PDFRenderKey target_key{i, target_state_.zoom, target_state_.rotate};
    auto &page = pages_[i];
    if (!page.hasTexture() || page.getKey() == target_key)
      continue;

    const int delta = (target_state_.rotate - page.getKey().rotate + 4) % 4;
    const auto current = page.getTextureSize();
    const auto target = scheduler_.getPageSize(target_key);

    const float target_w = (delta % 2 == 0) ? static_cast<float>(target.x)
                                            : static_cast<float>(target.y);
    const float target_h = (delta % 2 == 0) ? static_cast<float>(target.y)
                                            : static_cast<float>(target.x);

    const float scale_x = current.x ? target_w / static_cast<float>(current.x) : 1.f;
    const float scale_y = current.y ? target_h / static_cast<float>(current.y) : 1.f;

    if (i != anchor_page_) {
      page.setScale({scale_x, scale_y});
      page.setRotation(delta);
      page.syncPageShape(target_state_.rotate);
      continue;
    }

    const sf::Vector2f window_center = {window_size_.x * 0.5f, window_size_.y * 0.5f};
    const auto &local_focus = page.getSprite().getInverseTransform().transformPoint(window_center);

    page.setScale({scale_x, scale_y});
    page.setRotation(delta);
    page.syncPageShape(target_state_.rotate);

    if (delta == 0) {
      const auto &focus_after = page.getSprite().getTransform().transformPoint(local_focus);
      const auto &offset = window_center - focus_after;
      page.move(offset);
    }
  }
}

void PDFView::renderRequestedPages() {
  for (std::size_t i = front_page_; i <= back_page_; i++) {
    pdf::PDFRenderKey target_key = {i, target_state_.zoom, target_state_.rotate};
    if (!scheduler_.isReady(target_key))
      continue;

    if (pages_[i].hasTexture() && pages_[i].getKey() == target_key) {
      pages_[i].syncPageShape(target_state_.rotate);
      continue;
    }

    pages_[i].setTexture(*scheduler_.getTexture(target_key));
    pages_[i].setKey(target_key);

    pages_[i].setScale({1.f, 1.f});
    pages_[i].setRotation(0);

    sync_state_dirty_ = true;
    page_positions_dirty_ = true;
  }
}

void PDFView::updatePagePositions() {
  if (!pages_[anchor_page_].hasTexture())
    return;

  PageView &curr = pages_[anchor_page_];

  if (window_size_changed_) {
    const sf::Vector2f new_center{window_size_.x * 0.5f, window_size_.y * 0.5f};
    const sf::Vector2f curr_size = curr.getGlobalBounds().size;

    if (curr_size.x <= window_size_.x) {
      curr.setPosition({new_center.x, curr.getPosition().y});
    } else {
      const sf::Vector2f old_center{old_window_size_.x * 0.5f, old_window_size_.y * 0.5f};
      const auto &local_focus = curr.getSprite().getInverseTransform().transformPoint(old_center);
      const auto &focus_after = curr.getSprite().getTransform().transformPoint(local_focus);
      curr.move(new_center - focus_after);
    }

    window_size_changed_ = false;
  }

  clampAnchorHorizontally();
  discretizePosition(curr);
  updateNeighbourPositions();

  const sf::Vector2f front_size = pages_[front_page_].getGlobalBounds().size;
  const sf::Vector2f back_size = pages_[back_page_].getGlobalBounds().size;

  const float front_top = pages_[front_page_].getPosition().y - front_size.y * 0.5f;
  const float back_bottom = pages_[back_page_].getPosition().y + back_size.y * 0.5f;
  const float bottom_limit = window_size_.y - utils::cmdline_height_;
  const float curr_y = curr.getPosition().y;

  const float total_height = back_bottom - front_top;
  if (total_height < bottom_limit) {
    const float offset = (bottom_limit * 0.5f) - (front_top + total_height * 0.5f);
    curr.setPosition({curr.getPosition().x, curr_y + offset});
  } else if (front_top > 0.f) {
    curr.setPosition({curr.getPosition().x, curr_y - front_top});
  } else if (back_bottom < bottom_limit) {
    curr.setPosition({curr.getPosition().x, curr_y + (bottom_limit - back_bottom)});
  } else {
    discretizePosition(curr);
    return;
  }

  discretizePosition(curr);
  updateNeighbourPositions();
}

void PDFView::checkForAnchorPage() {
  const sf::Vector2f window_center = {window_size_.x * 0.5f, window_size_.y * 0.5f};

  std::size_t closest = anchor_page_;
  float closest_dist = std::numeric_limits<float>::max();

  for (std::size_t i = front_page_; i <= back_page_; i++) {
    if (!pages_[i].hasTexture())
      continue;
    const float page_center_y = pages_[i].getPosition().y;
    const float dist = std::abs(page_center_y - window_center.y);
    if (dist < closest_dist) {
      closest_dist = dist;
      closest = i;
    }
  }

  if (closest != anchor_page_ && started_scrolling_) {
    anchor_page_ = closest;
    requestPage(anchor_page_, target_state_.zoom, target_state_.rotate);
    event_bus_.emit("statusbar.page_state", std::make_pair(anchor_page_ + 1, document_.pageCount()));
  }
}

void PDFView::setZoom(float new_zoom) {
  if (!has_document_)
    return;

  const float clamped = std::clamp(new_zoom, min_zoom_, max_zoom_);
  if (clamped == target_state_.zoom)
    return;

  event_bus_.emit("statusbar.page_zoom", clamped);
  target_state_.zoom = clamped;
  scale_rot_update_timer_.restart();
  pending_page_update_ = true;

  sync_state_dirty_ = true;
}

void PDFView::setRotate(int rotate) {
  if (!has_document_)
    return;

  const int clamped = ((rotate % 4) + 4) % 4;
  if (clamped == target_state_.rotate)
    return;

  target_state_.rotate = clamped;
  scale_rot_update_timer_.restart();
  pending_page_update_ = true;

  sync_state_dirty_ = true;
}

void PDFView::updateNeighbourPositions() {
  if (anchor_page_ > front_page_) {
    sf::Vector2f below_size = pages_[anchor_page_].getGlobalBounds().size;
    for (int i = static_cast<int>(anchor_page_) - 1; i >= static_cast<int>(front_page_); --i) {
      const auto below_pos = pages_[i + 1].getPosition();
      const sf::Vector2f curr_size = pages_[i].getGlobalBounds().size;

      pages_[i].setPosition(
          {below_pos.x, below_pos.y - below_size.y * 0.5f - gap_ - curr_size.y * 0.5f}
      );
      below_size = curr_size;
    }
  }

  if (anchor_page_ < back_page_) {
    sf::Vector2f above_size = pages_[anchor_page_].getGlobalBounds().size;
    for (std::size_t i = anchor_page_ + 1; i <= back_page_; ++i) {
      const auto above_pos = pages_[i - 1].getPosition();
      const sf::Vector2f curr_size = pages_[i].getGlobalBounds().size;

      pages_[i].setPosition(
          {above_pos.x, above_pos.y + above_size.y * 0.5f + gap_ + curr_size.y * 0.5f}
      );
      above_size = curr_size;
    }
  }
}

void PDFView::clampAnchorHorizontally() {
  auto &page = pages_[anchor_page_];

  const float window_w = window_size_.x;
  const float page_w = static_cast<float>(
      scheduler_.getPageSize({page_with_max_width_, target_state_.zoom, target_state_.rotate}).x
  );
  const float half_w = page_w * 0.5f;

  sf::Vector2f pos = page.getPosition();

  if (page_w <= window_w) {
    pos.x = window_w * 0.5f;
  } else {
    const float min_x = window_w - half_w;
    const float max_x = half_w;
    pos.x = std::clamp(pos.x, min_x, max_x);
  }

  page.setPosition(pos);
}

void PDFView::discretizePosition(PageView &page) {
  const auto &loc = page.getPosition();
  page.setPosition({0.f, 0.f});
  const sf::Vector2f offset = page.getSprite().getTransform().transformPoint({0.f, 0.f});

  const float x = std::round(loc.x + offset.x) - offset.x;
  const float y = std::round(loc.y + offset.y) - offset.y;
  page.setPosition({x, y});
}

float PDFView::map(float value, float src_min, float src_max, float dst_min, float dst_max) {
  float t = (value - src_min) / (src_max - src_min);
  return std::lerp(dst_min, dst_max, t);
}

void PDFView::panCurrentPage(sf::Vector2f delta) {
  if (!has_document_)
    return;

  if (!pages_[anchor_page_].hasTexture())
    return;

  if (std::abs(delta.x) < epsilon_) {
    anchor_page_pos_before_scroll_ = pages_[anchor_page_].getPosition();
    started_scrolling_ = true;
  }

  pages_[anchor_page_].move(delta);
  page_positions_dirty_ = true;
}

void PDFView::goNextPageWithResult() {
  if (total_search_results_ == 0)
    return;

  pages_[anchor_page_].setSelectedSearchResult(-1);

  if (local_search_result_ + 1 < pages_[anchor_page_].getSearchResults().local_rects.size()) {
    curr_search_result_++;
    local_search_result_++;
    pages_[anchor_page_].setSelectedSearchResult(local_search_result_);

    event_bus_
        .emit("statusbar.search_state", std::make_pair(curr_search_result_ + 1, total_search_results_));
    search_pos_dirty_ = true;
    show_search_result_boxes_ = true;
    return;
  }

  auto it = std::
      upper_bound(pages_with_search_results_.begin(), pages_with_search_results_.end(), static_cast<int>(anchor_page_));
  if (it == pages_with_search_results_.end())
    it = pages_with_search_results_.begin();

  curr_search_result_ = pages_[*it].getSearchResults().start;
  local_search_result_ = 0;
  onSwitchPage(*it);

  pages_[anchor_page_].setSelectedSearchResult(local_search_result_);
  event_bus_
      .emit("statusbar.search_state", std::make_pair(curr_search_result_ + 1, total_search_results_));
  search_pos_dirty_ = true;
  show_search_result_boxes_ = true;
}

void PDFView::goPrevPageWithResult() {
  if (total_search_results_ == 0)
    return;

  pages_[anchor_page_].setSelectedSearchResult(-1);

  if (local_search_result_ > 0) {
    curr_search_result_--;
    local_search_result_--;
    pages_[anchor_page_].setSelectedSearchResult(local_search_result_);

    event_bus_
        .emit("statusbar.search_state", std::make_pair(curr_search_result_ + 1, total_search_results_));
    search_pos_dirty_ = true;
    show_search_result_boxes_ = true;
    return;
  }

  auto it = std::
      lower_bound(pages_with_search_results_.begin(), pages_with_search_results_.end(), static_cast<int>(anchor_page_));
  if (it == pages_with_search_results_.begin())
    it = pages_with_search_results_.end();
  --it;

  const std::size_t last_local = pages_[*it].getSearchResults().local_rects.size() - 1;
  curr_search_result_ = pages_[*it].getSearchResults().start + last_local;
  local_search_result_ = last_local;
  onSwitchPage(*it);
  pages_[anchor_page_].setSelectedSearchResult(local_search_result_);

  event_bus_
      .emit("statusbar.search_state", std::make_pair(curr_search_result_ + 1, total_search_results_));
  search_pos_dirty_ = true;
  show_search_result_boxes_ = true;
}

void PDFView::requestPage(std::size_t page_idx, float zoom, int rotate) {
  if (!has_document_)
    return;

  pdf::PDFRenderKey center_key{page_idx, zoom, rotate};
  const sf::Vector2u page_dims = scheduler_.getPageSize(center_key);

  int total_height = static_cast<int>(page_dims.y);
  int front = static_cast<int>(page_idx);
  int back = static_cast<int>(page_idx);

  while (total_height <= window_size_.y) {
    bool expanded = false;

    if (back + 1 < static_cast<int>(document_.pageCount())) {
      back++;
      pdf::PDFRenderKey key{static_cast<size_t>(back), zoom, rotate};
      total_height += scheduler_.getPageSize(key).y;
      expanded = true;
    }

    if (front > 0) {
      --front;
      pdf::PDFRenderKey key{static_cast<size_t>(front), zoom, rotate};
      total_height += scheduler_.getPageSize(key).y;
      expanded = true;
    }

    if (!expanded)
      break;
  }

  for (int i = 0; i < 3; i++) {
    if (back + 1 < static_cast<int>(document_.pageCount()))
      ++back;

    if (front > 0)
      --front;
  }

  for (std::size_t i = front_page_; i <= back_page_; ++i) {
    if (static_cast<int>(i) < front || static_cast<int>(i) > back)
      pages_[i].reset();
  }

  for (int i = front; i <= back; ++i) {
    pdf::PDFRenderKey key{static_cast<std::size_t>(i), zoom, rotate};
    if (pages_[i].hasTexture() && pages_[i].getKey() == key)
      continue;

    scheduler_.request(key);
    if (show_search_result_boxes_)
      pages_[i].showSearchResults();
  }

  front_page_ = static_cast<std::size_t>(front);
  back_page_ = static_cast<std::size_t>(back);
}

void PDFView::resetView() {
  pages_.clear();
  pages_.resize(document_.pageCount(), dummy_);
  for (std::size_t i = 0; i < document_.pageCount(); i++) {
    const auto &bounds = document_.getPage(i).page_bounds;
    float width = bounds.x1 - bounds.x0;
    float height = bounds.y1 - bounds.y0;
    pages_[i].setPageShapeSize({width, height});
  }

  need_initial_pos_ = true;
  window_size_changed_ = false;
  page_positions_dirty_ = false;
  sync_state_dirty_ = false;

  page_with_max_width_ = 0;

  front_page_ = 0;
  anchor_page_ = 0;
  back_page_ = 0;

  target_state_.zoom = 1.f;
  target_state_.rotate = 0;
}

} // namespace ui
