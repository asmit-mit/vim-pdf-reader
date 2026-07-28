#include <stdexcept>

#include "ui/pdf_view.h"
#include "ui/ui_elements.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

PDFView::PDFView(
    pdf::PDFDocument &document, core::RenderScheduler &scheduler, core::EventBus &event_bus
)
    : event_bus_(event_bus), document_(document), scheduler_(scheduler),
      sprite_(sf::Sprite(texture_)) {
  horizontal_wheel_.setFillColor(utils::hexToRGB(settings::scrollwheel_color_));
  vertical_wheel_.setFillColor(utils::hexToRGB(settings::scrollwheel_color_));

  event_bus_
      .subscribe<std::string>("cmd_processor.open_document", [this](const std::string &filepath) {
        onOpenDocument(filepath);
      });

  event_bus_.subscribe<bool>("cmd_processor.close_document", [this](bool) { onCloseDocument(); });

  event_bus_.subscribe<int>("cmd_processor.switch_page", [this](int page_num) {
    onSwitchPage(page_num);
  });

  event_bus_.subscribe<ui::UIElements>("ui.focus", [this](ui::UIElements focus) {
    should_take_input_ = focus == ui::UIElements::PDFView;
  });
}

void PDFView::draw(sf::RenderTarget &window) const {
  if (!has_document_)
    return;
  window.draw(sprite_);
  horizontal_wheel_.draw(window);
  vertical_wheel_.draw(window);
}

void PDFView::update() {
  if (!has_document_)
    return;

  requestRenderIfChanged();
  applyReadyRender();
  updateSpriteTransform();
  refreshScrollbars();

  horizontal_wheel_.update();
  vertical_wheel_.update();
}

void PDFView::handleEvent(const sf::Event &event) {
  if (!has_document_)
    return;

  const auto *key = event.getIf<sf::Event::KeyPressed>();
  if (key && should_take_input_) {
    if (key->code == sf::Keyboard::Key::Equal && key->control)
      setZoom(visual_.zoom + settings::delta_zoom_);
    else if (key->code == sf::Keyboard::Key::Hyphen && key->control)
      setZoom(visual_.zoom - settings::delta_zoom_);
    else if (key->code == sf::Keyboard::Key::Equal)
      setZoom(1.f);
    else if (key->code == sf::Keyboard::Key::R) {
      if (key->shift)
        setRotate(visual_.rotate - 1);
      else
        setRotate(visual_.rotate + 1);
    } else if (key->code == sf::Keyboard::Key::G) {
      if (key->shift) {
        setPageLoc(page_loc_.x, window_size_.y - (sprite_.getGlobalBounds().size.y / 2.f) - utils::cmdline_height_);
      } else if (prev_key_ == sf::Keyboard::Key::G) {
        setPageLoc(page_loc_.x, sprite_.getGlobalBounds().size.y / 2.f);
        prev_key_ = sf::Keyboard::Key::Unknown;
      } else
        prev_key_ = sf::Keyboard::Key::G;
    } else if (key->code == sf::Keyboard::Key::J)
      setPageLoc(page_loc_.x, page_loc_.y - scroll_dist_);
    else if (key->code == sf::Keyboard::Key::K)
      setPageLoc(page_loc_.x, page_loc_.y + scroll_dist_);
    else if (key->code == sf::Keyboard::Key::H)
      setPageLoc(page_loc_.x + scroll_dist_, page_loc_.y);
    else if (key->code == sf::Keyboard::Key::L)
      setPageLoc(page_loc_.x - scroll_dist_, page_loc_.y);
    else if (key->code == sf::Keyboard::Key::U) {
      if (key->control)
        setPageLoc(page_loc_.x, page_loc_.y + window_size_.y * 0.5f);
      else
        current_page_ = std::max(0, (int)current_page_ - 1);
    } else if (key->code == sf::Keyboard::Key::D) {
      if (key->control)
        setPageLoc(page_loc_.x, page_loc_.y - window_size_.y * 0.5f);
      else
        current_page_ = std::min(document_.size() - 1, current_page_ + 1);
    } else if (key->code == sf::Keyboard::Key::F) {
      const auto bounds = sprite_.getGlobalBounds();
      const float width_zoom = visual_.zoom * window_size_.x / bounds.size.x;
      const float height_zoom = visual_.zoom * (window_size_.y - utils::cmdline_height_) /
                                bounds.size.y;
      setZoom(std::min(width_zoom, height_zoom));
    } else if (key->code == sf::Keyboard::Key::W)
      setZoom(visual_.zoom * window_size_.x / sprite_.getGlobalBounds().size.x);
  }
}

void PDFView::onResize(const sf::Vector2f &size) {
  window_size_ = size;
  setPageLoc(page_loc_.x, page_loc_.y);
}

void PDFView::onOpenDocument(const std::string &filepath) {
  try {
    scheduler_.quiesce();
    document_.openDocument(utils::resolvePath(filepath));
    scheduler_.clearCache();
    scheduler_.resume();

    resetView();
    has_document_ = true;
    needs_initial_center_ = true;

    requestPage(current_page_, render_.zoom, render_.rotate);

    event_bus_.emit("statusbar.pdf_path", utils::resolvePath(filepath));
    event_bus_.emit("statusbar.page_number", current_page_ + 1);
    event_bus_.emit("statusbar.total_pages", document_.size());
    event_bus_.emit("statusbar.page_zoom", visual_.zoom);
  } catch (const std::runtime_error &e) {
    has_document_ = false;
    event_bus_.emit("cmdline.msg", e.what());
  }
}

void PDFView::onCloseDocument() {
  if (!has_document_)
    return;
  scheduler_.clearCache();
  document_.closeDocument();
  resetView();
  event_bus_.emit("statusbar.pdf_path", std::string("[Nothing Open Yet]"));
  event_bus_.emit("statusbar.total_pages", (std::size_t)0);
}

void PDFView::onSwitchPage(int page_num) {
  if (!has_document_) {
    const char *msg = "No document currently open";
    event_bus_.emit("cmdline.msg", msg);
    return;
  }
  if (page_num < 1 || page_num > (int)document_.size()) {
    const char *msg = "Page number out of range";
    event_bus_.emit("cmdline.msg", msg);
    return;
  }
  current_page_ = page_num - 1;
}

void PDFView::requestRenderIfChanged() {
  const bool zoom_or_rotate_changed = render_.zoom != visual_.zoom ||
                                      render_.rotate != visual_.rotate;
  const bool debounce_elapsed = page_update_timer_.getElapsedTime().asMilliseconds() >
                                zoom_debounce_ms_;

  if (zoom_or_rotate_changed && debounce_elapsed)
    requestPage(current_page_, visual_.zoom, visual_.rotate);

  if (render_page_ != current_page_) {
    requestPage(current_page_, visual_.zoom, visual_.rotate);
    event_bus_.emit("statusbar.page_number", current_page_ + 1);
  }
}

void PDFView::applyReadyRender() {
  if (!pending_.active || !scheduler_.isReady(pending_.key))
    return;

  sf::Texture *tex = scheduler_.getTexture(pending_.key);
  if (!tex)
    return;

  texture_ = *tex;
  texture_.setSmooth(false);
  sprite_.setTexture(texture_, true);
  sprite_.setScale({1.f, 1.f});
  pending_.active = false;

  render_page_ = current_page_;
  render_.zoom = visual_.zoom;
  render_.rotate = visual_.rotate;

  if (needs_initial_center_) {
    centerPage();
    setPageLoc(window_size_.x * 0.5f, window_size_.y * 0.5f);
    needs_initial_center_ = false;
  }
}

void PDFView::updateSpriteTransform() {
  syncScaleRotation();
  centerPage();

  sprite_.setPosition({0.f, 0.f});
  const sf::Vector2f corner_offset = sprite_.getTransform().transformPoint({0.f, 0.f});

  const float pos_x = std::round(page_loc_.x + corner_offset.x) - corner_offset.x;
  const float pos_y = std::round(page_loc_.y + corner_offset.y) - corner_offset.y;
  sprite_.setPosition({pos_x, pos_y});
}

void PDFView::refreshScrollbars() {
  if (scrollbar_dirty_.horizontal) {
    const auto bounds = sprite_.getGlobalBounds();
    const float page_w = bounds.size.x;
    const float scroll_width = (window_size_.x * window_size_.x) / page_w;
    horizontal_wheel_.setSize({scroll_width, scrollwheel_width_});

    const float scroll_x =
        map(page_loc_.x,
            window_size_.x - page_w / 2.f,
            page_w / 2.f,
            window_size_.x - scroll_width,
            0.f);

    horizontal_wheel_.setPosition(
        {scroll_x, window_size_.y - utils::cmdline_height_ - scrollwheel_width_}
    );
    scrollbar_dirty_.horizontal = false;
  }

  if (scrollbar_dirty_.vertical) {
    const auto bounds = sprite_.getGlobalBounds();
    const float page_h = bounds.size.y;
    const float scroll_height = (window_size_.y * window_size_.y) / page_h;
    vertical_wheel_.setSize({scrollwheel_width_, scroll_height});

    const float scroll_y =
        map(page_loc_.y,
            window_size_.y - page_h / 2.f - utils::cmdline_height_,
            page_h / 2.f,
            window_size_.y - utils::cmdline_height_ - scroll_height,
            0.f);

    vertical_wheel_.setPosition({window_size_.x - scrollwheel_width_, scroll_y});
    scrollbar_dirty_.vertical = false;
  }
}

void PDFView::setZoom(float new_zoom) {
  if (!has_document_)
    return;

  const float clamped = std::clamp(new_zoom, min_zoom_, max_zoom_);
  if (clamped == visual_.zoom)
    return;

  event_bus_.emit("statusbar.page_zoom", clamped);
  visual_.zoom = clamped;
  page_update_timer_.restart();
  setPageLoc(page_loc_.x, page_loc_.y);

  const auto bounds = sprite_.getGlobalBounds();
  scrollbar_dirty_.horizontal = bounds.size.x > window_size_.x;
  scrollbar_dirty_.vertical = bounds.size.y > window_size_.y;
}

void PDFView::setRotate(int rotate) {
  if (!has_document_)
    return;

  visual_.rotate = (rotate % 4 + 4) % 4;
  page_update_timer_.restart();
  setPageLoc(page_loc_.x, page_loc_.y);

  const auto bounds = sprite_.getGlobalBounds();
  scrollbar_dirty_.horizontal = bounds.size.x > window_size_.x;
  scrollbar_dirty_.vertical = bounds.size.y > window_size_.y;
}

float PDFView::map(float value, float src_min, float src_max, float dst_min, float dst_max) {
  float t = (value - src_min) / (src_max - src_min);
  return std::lerp(dst_min, dst_max, t);
}

void PDFView::requestPage(std::size_t page_num, float zoom, int rotate) {
  pdf::PDFRenderKey key{page_num, zoom, rotate};
  scheduler_.request(key);
  pending_.key = key;
  pending_.active = true;
}

void PDFView::syncScaleRotation() {
  if (!size_cache_.valid || size_cache_.page != current_page_ || size_cache_.zoom != visual_.zoom ||
      size_cache_.rotate != visual_.rotate) {
    const pdf::PDFRenderKey size_key{current_page_, visual_.zoom, visual_.rotate};
    size_cache_.size = scheduler_.getPageSize(size_key);
    size_cache_.page = current_page_;
    size_cache_.zoom = visual_.zoom;
    size_cache_.rotate = visual_.rotate;
    size_cache_.valid = true;
  }

  const int delta = (visual_.rotate - render_.rotate + 4) % 4;

  const sf::Vector2u &target = size_cache_.size;
  const sf::Vector2u current = texture_.getSize();

  const float target_w = (delta % 2 == 0) ? static_cast<float>(target.x)
                                          : static_cast<float>(target.y);
  const float target_h = (delta % 2 == 0) ? static_cast<float>(target.y)
                                          : static_cast<float>(target.x);

  const float scale_x = current.x > 0 ? target_w / static_cast<float>(current.x) : 1.f;
  const float scale_y = current.y > 0 ? target_h / static_cast<float>(current.y) : 1.f;

  sprite_.setScale({scale_x, scale_y});
  sprite_.setRotation(sf::degrees(delta * 90.f));
}

void PDFView::setPageLoc(float x, float y) {
  if (!has_document_) {
    page_loc_ = {x, y};
    return;
  }

  syncScaleRotation();

  const auto bounds = sprite_.getGlobalBounds();
  const float page_w = bounds.size.x;
  const float page_h = bounds.size.y;
  const float viewport_h = window_size_.y - utils::cmdline_height_;

  if (page_w <= window_size_.x)
    x = window_size_.x * 0.5f;
  else
    x = std::clamp(x, window_size_.x - page_w * 0.5f, page_w * 0.5f);

  if (page_h <= viewport_h)
    y = viewport_h * 0.5f;
  else
    y = std::clamp(y, viewport_h - page_h * 0.5f, page_h * 0.5f);

  scrollbar_dirty_.horizontal = page_loc_.x != x && page_w > window_size_.x;
  scrollbar_dirty_.vertical = page_loc_.y != y && page_h > viewport_h;

  page_loc_ = {x, y};
}

void PDFView::centerPage() {
  const auto bounds = sprite_.getLocalBounds();
  sprite_.setOrigin(bounds.getCenter());
}

void PDFView::resetView() {
  has_document_ = false;

  current_page_ = 0;
  render_page_ = 0;

  visual_.zoom = 1.f;
  render_.zoom = 1.f;
  visual_.rotate = 0;
  render_.rotate = 0;

  size_cache_.valid = false;
  pending_.active = false;
  needs_initial_center_ = false;

  setPageLoc(0.f, 0.f);
  texture_ = sf::Texture{};
  sprite_.setTexture(texture_, true);
}

} // namespace ui
