#include <print>
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
  // horizontal_wheel_.setFillColor(utils::hexToRGB(settings::scrollwheel_color_));
  // vertical_wheel_.setFillColor(utils::hexToRGB(settings::scrollwheel_color_));

  has_document_ = false;
  should_take_input_ = false;
  need_initial_pos_ = false;
  window_size_changed_ = false;
  pending_page_update_ = false;

  front_page_ = 0;
  curr_page_ = 0;
  back_page_ = 0;

  event_bus_
      .subscribe<std::string>("cmd_processor.open_document", [this](const std::string &filepath) {
        onOpenDocument(filepath);
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

  for (std::size_t i = front_page_; i <= back_page_; i++)
    pages_[i].draw(window);
}

void PDFView::update() {
  if (!has_document_)
    return;

  if (pending_page_update_ &&
      scale_rot_update_timer_.getElapsedTime().asMilliseconds() > scale_rot_debounce_ms_) {
    requestPage(curr_page_, target_state_.zoom, target_state_.rotate);
    std::println("requested new pages for pending updates");
    pending_page_update_ = false;
  }

  setInitialPagePos();
  syncWithTargetState();
  renderRequestedPages();
  updatePagePositions();
  checkForCurrentPage();
}

void PDFView::handleEvent(const sf::Event &event) {
  if (!has_document_)
    return;

  const auto *key = event.getIf<sf::Event::KeyPressed>();
  if (key && should_take_input_) {
    if (key->code == sf::Keyboard::Key::U) {
      onSwitchPage(std::max(0, (int)curr_page_ - 1));
    } else if (key->code == sf::Keyboard::Key::D) {
      onSwitchPage(std::min(document_.size() - 1, curr_page_ + 1));
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
    } else if (key->code == sf::Keyboard::Key::R && key->shift) {
      setRotate((target_state_.rotate + 1) % 4);
    }
  }
}

void PDFView::onResize(const sf::Vector2f &size) {
  window_size_ = size;
  window_size_changed_ = true;
}

void PDFView::onOpenDocument(const std::string &filepath) {
  try {
    scheduler_.quiesce();
    document_.openDocument(utils::resolvePath(filepath));
    scheduler_.clearCache();
    scheduler_.resume();

    has_document_ = true;
    resetView();
    onSwitchPage(curr_page_);

    event_bus_.emit("statusbar.pdf_path", utils::resolvePath(filepath));
    event_bus_.emit("statusbar.page_number", curr_page_ + 1);
    event_bus_.emit("statusbar.total_pages", document_.size());
    event_bus_.emit("statusbar.page_zoom", target_state_.zoom);
  } catch (const std::runtime_error &e) {
    has_document_ = false;
    event_bus_.emit("cmdline.msg", e.what());
  }
}

void PDFView::onCloseDocument() {
  if (!has_document_)
    return;
  resetView();
  document_.closeDocument();
  event_bus_.emit("statusbar.pdf_path", std::string("[Nothing Open Yet]"));
  event_bus_.emit("statusbar.total_pages", static_cast<size_t>(0));
  has_document_ = false;
}

void PDFView::onSwitchPage(int page_idx) {
  if (!has_document_) {
    const char *msg = "No document currently open";
    event_bus_.emit("cmdline.msg", msg);
    return;
  }
  if (page_idx < 0 || page_idx >= static_cast<int>(document_.size())) {
    const char *msg = "Page number out of range";
    event_bus_.emit("cmdline.msg", msg);
    return;
  }
  curr_page_ = static_cast<std::size_t>(page_idx);
  requestPage(curr_page_, target_state_.zoom, target_state_.rotate);
  event_bus_.emit("statusbar.page_number", curr_page_ + 1);
  need_initial_pos_ = true;
}

void PDFView::setInitialPagePos() {
  if (!need_initial_pos_)
    return;

  if (!pages_[curr_page_].hasTexture())
    return;

  const auto &size = pages_[curr_page_].getGlobalBounds().size;
  float x = pages_[curr_page_].getPosition().x;
  if (size.x <= window_size_.x)
    x = (window_size_.x - size.x) * 0.5f;
  float y = (window_size_.y - size.y) * 0.5f - utils::cmdline_height_;
  if (size.y >= window_size_.y)
    y = 0.f;

  pages_[curr_page_].setPosition({std::round(x), std::round(y)});

  need_initial_pos_ = false;
}

void PDFView::syncWithTargetState() {
  for (std::size_t i = front_page_; i <= back_page_; i++) {
    pdf::PDFRenderKey target_key = {i, target_state_.zoom, target_state_.rotate};
    auto &page = pages_[i];
    if (page.hasTexture() && page.getKey() != target_key) {
      const int delta = (target_state_.rotate - page.getKey().rotate + 4) % 4;

      const auto &current = page.getSize();
      const auto target = scheduler_.getPageSize(target_key);

      const float target_w = (delta % 2 == 0) ? static_cast<float>(target.x)
                                              : static_cast<float>(target.y);
      const float target_h = (delta % 2 == 0) ? static_cast<float>(target.y)
                                              : static_cast<float>(target.x);

      const float scale_x = current.x > 0 ? target_w / static_cast<float>(current.x) : 1.f;
      const float scale_y = current.y > 0 ? target_h / static_cast<float>(current.y) : 1.f;

      if (i != curr_page_) {
        page.setScale({scale_x, scale_y});
        page.setRotation(target_state_.rotate);
        continue;
      }

      const sf::Vector2f window_center = {window_size_.x * 0.5f, window_size_.y * 0.5f};
      const auto &local_focus = page.getSprite().getInverseTransform().transformPoint(window_center);

      page.setScale({scale_x, scale_y});
      page.setRotation(target_state_.rotate);

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
  }
}

void PDFView::updatePagePositions() {
  if (!pages_[curr_page_].hasTexture())
    return;

  const auto &pos = pages_[curr_page_].getPosition();
  const auto &size = pages_[curr_page_].getGlobalBounds().size;
  if (size.x <= window_size_.x) {
    const float x = std::round((window_size_.x - size.x) * 0.5f);
    pages_[curr_page_].setPosition({x, pos.y});
    window_size_changed_ = false;
  }

  for (int i = static_cast<int>(curr_page_) - 1; i >= static_cast<int>(front_page_); --i) {
    const auto &below_pos = pages_[i + 1].getPosition();
    const auto curr_size = pages_[i].getGlobalBounds().size;

    float x = below_pos.x;
    if (curr_size.x <= window_size_.x)
      x = (window_size_.x - curr_size.x) * 0.5f;
    float y = below_pos.y - curr_size.y - gap_;

    pages_[i].setPosition({std::round(x), std::round(y)});
  }

  for (std::size_t i = curr_page_ + 1; i <= back_page_; ++i) {
    const auto curr_size = pages_[i].getGlobalBounds().size;
    const auto above_pos = pages_[i - 1].getPosition();
    const auto above_size = pages_[i - 1].getGlobalBounds().size;

    float x = above_pos.x;
    if (curr_size.x <= window_size_.x)
      x = (window_size_.x - curr_size.x) * 0.5f;

    float y = above_pos.y + above_size.y + gap_;

    pages_[i].setPosition({std::round(x), std::round(y)});
  }
}

void PDFView::checkForCurrentPage() {}

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
}

float PDFView::map(float value, float src_min, float src_max, float dst_min, float dst_max) {
  float t = (value - src_min) / (src_max - src_min);
  return std::lerp(dst_min, dst_max, t);
}

void PDFView::panCurrentPage(sf::Vector2f delta) {
  if (!has_document_ || !pages_[curr_page_].hasTexture())
    return;

  const auto &size = pages_[curr_page_].getSize();
  if (size.x <= window_size_.x)
    delta.x = 0.f;
  if (delta.x == 0.f && delta.y == 0.f)
    return;

  const auto pos = pages_[curr_page_].getPosition();
  pages_[curr_page_].setPosition({std::round(pos.x + delta.x), std::round(pos.y + delta.y)});
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
    if (back + 1 < static_cast<int>(document_.size())) {
      back++;
      pdf::PDFRenderKey key{static_cast<size_t>(back), zoom, rotate};
      total_height += scheduler_.getPageSize(key).y;
    }

    if (front > 0) {
      --front;
      pdf::PDFRenderKey key{static_cast<size_t>(front), zoom, rotate};
      total_height += scheduler_.getPageSize(key).y;
    }
  }

  if (back + 1 < static_cast<int>(document_.size()))
    ++back;

  if (front > 0)
    --front;

  for (std::size_t i = front_page_; i <= back_page_; ++i)
    pages_[i].reset();

  for (int i = front; i <= back; ++i) {
    pdf::PDFRenderKey key{static_cast<std::size_t>(i), zoom, rotate};
    if (pages_[i].hasTexture() && pages_[i].getKey() == key)
      continue;

    scheduler_.request(key);
  }

  front_page_ = static_cast<std::size_t>(front);
  back_page_ = static_cast<std::size_t>(back);

  std::println("requested for pages {}..{}", front_page_, back_page_);
}

void PDFView::resetView() {
  pages_.clear();
  pages_.resize(document_.size(), dummy_);
  for (auto &page : pages_)
    page.reset();

  front_page_ = 0;
  curr_page_ = 0;
  back_page_ = 0;

  target_state_.zoom = 1.f;
  target_state_.rotate = 0;
}

} // namespace ui
