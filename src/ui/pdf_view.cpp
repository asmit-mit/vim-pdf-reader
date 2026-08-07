#include <stdexcept>

#include "pdf/pdf_renderer.h"
#include "ui/pdf_view.h"
#include "ui/ui_elements.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

PDFView::PDFView(
    pdf::PDFDocument &document, core::RenderScheduler &scheduler, core::EventBus &event_bus
)
    : event_bus_(event_bus), document_(document), scheduler_(scheduler) {
  has_document_ = false;
  should_take_input_ = false;
  need_initial_pos_ = false;
  window_size_changed_ = false;
  pending_page_update_ = false;
  started_scrolling_ = false;
  page_positions_dirty_ = false;
  sync_state_dirty_ = false;

  front_page_ = 0;
  anchor_page_ = 0;
  back_page_ = 0;
  anchor_page_pos_before_scroll_ = {0.f, 0.f};

  page_with_max_width_ = 0;

  event_bus_
      .subscribe<std::string>("cmd_processor.open_document", [this](const std::string &filepath) {
        onOpenDocument(filepath);
      });

  event_bus_.subscribe<bool>("cmd_processor.reload_document", [this](bool) {
    onOpenDocument(filepath_);
  });

  event_bus_.subscribe<bool>("cmd_processor.close_document", [this](bool) { onCloseDocument(); });

  event_bus_.subscribe<int>("cmd_processor.switch_page", [this](int page_num) {
    onSwitchPage(page_num - 1);
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
    page_positions_dirty_ = false;
  }

  if (started_scrolling_)
    checkForAnchorPage();
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
        onSwitchPage(std::min(document_.size() - 1, anchor_page_ + 1));
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
    scheduler_.quiesce();
    document_.openDocument(utils::resolvePath(filepath));
    filepath_ = filepath;
    scheduler_.clearCache();
    scheduler_.resume();

    has_document_ = true;
    resetView();
    onSwitchPage(anchor_page_);
    page_with_max_width_ = document_.pageWithMaxWidth();

    event_bus_.emit("statusbar.pdf_path", utils::resolvePath(filepath));
    event_bus_.emit("statusbar.page_number", anchor_page_ + 1);
    event_bus_.emit("statusbar.total_pages", document_.size());
    event_bus_.emit("statusbar.page_zoom", target_state_.zoom);
  } catch (const std::runtime_error &e) {
    onCloseDocument();
    throw e;
  }
}

void PDFView::onCloseDocument() {
  if (!has_document_)
    return;
  has_document_ = false;
  resetView();
  document_.closeDocument();
  event_bus_.emit("statusbar.pdf_path", std::string("[Nothing Open Yet]"));
  event_bus_.emit("statusbar.total_pages", static_cast<size_t>(0));
}

void PDFView::onSwitchPage(int page_idx) {
  if (!has_document_) {
    const char *msg = "No document currently open";
    event_bus_.emit("notification.msg", msg);
    return;
  }
  if (page_idx < 0 || page_idx >= static_cast<int>(document_.size())) {
    const char *msg = "Page number out of range";
    event_bus_.emit("notification.msg", msg);
    return;
  }
  anchor_page_ = static_cast<std::size_t>(page_idx);
  requestPage(anchor_page_, target_state_.zoom, target_state_.rotate);
  event_bus_.emit("statusbar.page_number", anchor_page_ + 1);
  need_initial_pos_ = true;
  started_scrolling_ = false;
}

void PDFView::setInitialPagePos() {
  if (!need_initial_pos_)
    return;

  if (!pages_[anchor_page_].hasTexture())
    return;

  const auto size = pages_[anchor_page_].getGlobalBounds().size;

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
      continue;
    }

    const sf::Vector2f window_center = {window_size_.x * 0.5f, window_size_.y * 0.5f};
    const auto &local_focus = page.getSprite().getInverseTransform().transformPoint(window_center);

    page.setScale({scale_x, scale_y});
    page.setRotation(delta);

    if (delta == 0) {
      const auto &focus_after = page.getSprite().getTransform().transformPoint(local_focus);
      const auto &offset = window_center - focus_after;
      page.getSprite().move(offset);
    }
  }
}

void PDFView::renderRequestedPages() {
  for (std::size_t i = front_page_; i <= back_page_; i++) {
    pdf::PDFRenderKey target_key = {i, target_state_.zoom, target_state_.rotate};
    if (!scheduler_.isReady(target_key))
      continue;

    if (pages_[i].hasTexture() && pages_[i].getKey() == target_key)
      continue;

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
      curr.getSprite().move(new_center - focus_after);
    }

    window_size_changed_ = false;
  }

  clampAnchorHorizontally();
  putPageInNonFracPos(curr);
  updateNeighbourPositions();

  // Cache bounds for front/back pages — each was called twice before.
  const sf::Vector2f front_size = pages_[front_page_].getGlobalBounds().size;
  const sf::Vector2f back_size = pages_[back_page_].getGlobalBounds().size;

  const float front_top = pages_[front_page_].getPosition().y - front_size.y * 0.5f;
  const float back_bottom = pages_[back_page_].getPosition().y + back_size.y * 0.5f;
  const float bottom_limit = window_size_.y - utils::cmdline_height_;
  const float curr_y = curr.getPosition().y;

  if (front_page_ == back_page_ && front_size.y < window_size_.y) {
    curr.setPosition({window_size_.x * 0.5f, window_size_.y * 0.5f - utils::cmdline_height_});
  } else if (front_top > 0.f) {
    curr.setPosition({curr.getPosition().x, curr_y - front_top});
  } else if (back_bottom < bottom_limit) {
    curr.setPosition({curr.getPosition().x, curr_y + (bottom_limit - back_bottom)});
  } else {
    // No clamping needed — neighbour positions are already up to date, skip second pass.
    putPageInNonFracPos(curr);
    return;
  }

  putPageInNonFracPos(curr);
  updateNeighbourPositions();
}

void PDFView::checkForAnchorPage() {
  const sf::Vector2f window_center = {window_size_.x * 0.5f, window_size_.y * 0.5f};
  for (std::size_t i = front_page_; i < back_page_; i++) {
    if (pages_[i].getSprite().getGlobalBounds().contains(window_center)) {
      if (anchor_page_ != i && (started_scrolling_ && pages_[anchor_page_].getPosition() !=
                                                          anchor_page_pos_before_scroll_)) {
        anchor_page_ = i;
        requestPage(anchor_page_, target_state_.zoom, target_state_.rotate);
        event_bus_.emit("statusbar.page_number", anchor_page_ + 1);
      }
    }
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
  // Backward pass: walk from anchor-1 toward front.
  // Seed with the anchor's bounds so the first iteration doesn't need a second lookup.
  if (anchor_page_ > front_page_) {
    sf::Vector2f below_size = pages_[anchor_page_].getGlobalBounds().size;
    for (int i = static_cast<int>(anchor_page_) - 1; i >= static_cast<int>(front_page_); --i) {
      const auto below_pos = pages_[i + 1].getPosition();
      const sf::Vector2f curr_size = pages_[i].getGlobalBounds().size;

      pages_[i].setPosition(
          {below_pos.x, below_pos.y - below_size.y * 0.5f - gap_ - curr_size.y * 0.5f}
      );
      below_size = curr_size; // carry forward: next iteration's "below" is this page
    }
  }

  // Forward pass: walk from anchor+1 toward back.
  if (anchor_page_ < back_page_) {
    sf::Vector2f above_size = pages_[anchor_page_].getGlobalBounds().size;
    for (std::size_t i = anchor_page_ + 1; i <= back_page_; ++i) {
      const auto above_pos = pages_[i - 1].getPosition();
      const sf::Vector2f curr_size = pages_[i].getGlobalBounds().size;

      pages_[i].setPosition(
          {above_pos.x, above_pos.y + above_size.y * 0.5f + gap_ + curr_size.y * 0.5f}
      );
      above_size = curr_size; // carry forward: next iteration's "above" is this page
    }
  }
}

void PDFView::clampAnchorHorizontally() {
  auto &sprite = pages_[anchor_page_].getSprite();

  const float window_w = window_size_.x;
  const float page_w = static_cast<float>(
      scheduler_.getPageSize({page_with_max_width_, target_state_.zoom, target_state_.rotate}).x
  );
  const float half_w = page_w * 0.5f;

  sf::Vector2f pos = sprite.getPosition();

  if (page_w <= window_w) {
    pos.x = window_w * 0.5f;
  } else {
    const float min_x = window_w - half_w;
    const float max_x = half_w;
    pos.x = std::clamp(pos.x, min_x, max_x);
  }

  sprite.setPosition(pos);
}

void PDFView::putPageInNonFracPos(PageView &page) {
  const auto &loc = page.getSprite().getPosition();
  page.getSprite().setPosition({0.f, 0.f});
  const sf::Vector2f offset = page.getSprite().getTransform().transformPoint({0.f, 0.f});

  const float x = std::round(loc.x + offset.x) - offset.x;
  const float y = std::round(loc.y + offset.y) - offset.y;
  page.getSprite().setPosition({x, y});
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

  pages_[anchor_page_].getSprite().move(delta);
  page_positions_dirty_ = true;
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

    if (back + 1 < static_cast<int>(document_.size())) {
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
    if (back + 1 < static_cast<int>(document_.size()))
      ++back;

    if (front > 0)
      --front;
  }

  // Only reset pages that are falling out of the new visible range.
  // Pages that remain within [front, back] keep their textures and positions.
  for (std::size_t i = front_page_; i <= back_page_; ++i) {
    if (static_cast<int>(i) < front || static_cast<int>(i) > back)
      pages_[i].reset();
  }

  for (int i = front; i <= back; ++i) {
    pdf::PDFRenderKey key{static_cast<std::size_t>(i), zoom, rotate};
    if (pages_[i].hasTexture() && pages_[i].getKey() == key)
      continue;

    scheduler_.request(key);
  }

  front_page_ = static_cast<std::size_t>(front);
  back_page_ = static_cast<std::size_t>(back);
}

void PDFView::resetView() {
  pages_.clear();
  pages_.resize(document_.size(), dummy_);

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
