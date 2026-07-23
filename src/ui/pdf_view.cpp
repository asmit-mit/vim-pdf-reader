#include <stdexcept>

#include "ui/pdf_view.h"
#include "ui/ui_elements.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

PDFView::PDFView(pdf::PDFDocument &document, pdf::PDFRenderer &renderer, core::EventBus &event_bus)
    : event_bus_(event_bus), document_(document), renderer_(renderer),
      sprite_(sf::Sprite(texture_)) {
  curr_x_ = 0.f;
  curr_y_ = 0.f;
  current_page_ = 0;
  render_page_ = 0;
  has_document_ = false;
  should_take_input_ = false;
  update_scroll_bar_horizontal_ = false;

  horizontal_wheel_.setFillColor(utils::hexToRGB(settings::scrollwheel_color_));
  vertical_wheel_.setFillColor(utils::hexToRGB(settings::scrollwheel_color_));

  event_bus_
      .subscribe<std::string>("cmd_processor.open_document", [this](const std::string &filepath) {
        try {
          document_.openDocument(utils::resolvePath(filepath));
          resetView();
          has_document_ = true;

          getPage(current_page_, render_zoom_, render_rotate_);
          setPageLoc(window_size_.x / 2.f, window_size_.y / 2.f);

          event_bus_.emit("statusbar.pdf_path", utils::resolvePath(filepath));
          event_bus_.emit("statusbar.page_number", 1);
          event_bus_.emit("statusbar.total_pages", document_.size());
          event_bus_.emit("statusbar.page_zoom", visual_zoom_);
        } catch (const std::runtime_error &e) {
          has_document_ = false;
          event_bus_.emit("cmdline.msg", e.what());
        }
      });

  event_bus_.subscribe<bool>("cmd_processor.close_document", [this](bool close_document) {
    if (!has_document_)
      return;
    document_.closeDocument();
    resetView();
    event_bus_.emit("statusbar.pdf_path", std::string("[Nothing Open Yet]"));
    event_bus_.emit("statusbar.total_pages", (std::size_t)0);
  });

  event_bus_.subscribe<int>("cmd_processor.switch_page", [this](int page_num) {
    if (!has_document_) {
      const char *msg = "No document currently open";
      event_bus_.emit("cmdline.msg", msg);
    }
    if (page_num < 1 || page_num > (int)document_.size()) {
      const char *msg = "Page number out of range";
      event_bus_.emit("cmdline.msg", msg);
    }
    current_page_ = page_num - 1;
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

  if ((render_zoom_ != visual_zoom_ || render_rotate_ != visual_rotate_) &&
      page_update_timer_.getElapsedTime().asMilliseconds() > zoom_dobounce_ms_) {
    getPage(current_page_, visual_zoom_, visual_rotate_);
    render_zoom_ = visual_zoom_;
    render_rotate_ = visual_rotate_;
  }

  if (render_page_ != current_page_) {
    getPage(current_page_, visual_zoom_, visual_rotate_);
    render_page_ = current_page_;
    render_zoom_ = visual_zoom_;
    render_rotate_ = visual_rotate_;
    event_bus_.emit("statusbar.page_number", current_page_ + 1);
  }

  const float scale = visual_zoom_ / render_zoom_;
  const int delta = (visual_rotate_ - render_rotate_ + 4) % 4;
  sprite_.setScale({scale, scale});
  sprite_.setRotation(sf::degrees(delta * 90.f));
  sprite_.setPosition({std::round(curr_x_), std::round(curr_y_)});

  if (update_scroll_bar_horizontal_) {
    const auto bounds = sprite_.getGlobalBounds();
    const float page_w = bounds.size.x;
    float scroll_width = (window_size_.x * window_size_.x) / (page_w);
    horizontal_wheel_.setSize({scroll_width, scrollwheel_width_});

    float scroll_x =
        map(curr_x_,
            window_size_.x - page_w / 2.f,
            page_w / 2.f,
            window_size_.x - scroll_width,
            0.f);

    horizontal_wheel_.setPosition(
        {scroll_x, window_size_.y - utils::cmdline_height_ - scrollwheel_width_}
    );
    update_scroll_bar_horizontal_ = false;
  }

  if (update_scroll_bar_vertical_) {
    const auto bounds = sprite_.getGlobalBounds();
    const float page_h = bounds.size.y;
    float scroll_height = (window_size_.y * window_size_.y) / (page_h);
    vertical_wheel_.setSize({scrollwheel_width_, scroll_height});

    float scroll_y =
        map(curr_y_,
            window_size_.y - page_h / 2.f - utils::cmdline_height_,
            page_h / 2.f,
            window_size_.y - utils::cmdline_height_ - scroll_height,
            0.f);

    vertical_wheel_.setPosition({window_size_.x - scrollwheel_width_, scroll_y});
    update_scroll_bar_vertical_ = false;
  }

  horizontal_wheel_.update();
  vertical_wheel_.update();
}

void PDFView::handleEvent(const sf::Event &event) {
  if (!has_document_)
    return;

  const auto *key = event.getIf<sf::Event::KeyPressed>();
  if (key && should_take_input_) {
    if (key->code == sf::Keyboard::Key::Equal && key->control)
      setZoom(visual_zoom_ + settings::delta_zoom_);
    else if (key->code == sf::Keyboard::Key::Hyphen && key->control)
      setZoom(visual_zoom_ - settings::delta_zoom_);
    else if (key->code == sf::Keyboard::Key::Equal)
      setZoom(1.f);
    else if (key->code == sf::Keyboard::Key::R) {
      if (key->shift)
        setRotate(visual_rotate_ - 1);
      else
        setRotate(visual_rotate_ + 1);
    } else if (key->code == sf::Keyboard::Key::G) {
      if (key->shift) {
        setPageLoc(curr_x_, window_size_.y - (sprite_.getGlobalBounds().size.y / 2.f) - utils::cmdline_height_);
      } else if (prev_key_ == sf::Keyboard::Key::G) {
        setPageLoc(curr_x_, sprite_.getGlobalBounds().size.y / 2.f);
        prev_key_ = sf::Keyboard::Key::Unknown;
      } else
        prev_key_ = sf::Keyboard::Key::G;
    } else if (key->code == sf::Keyboard::Key::J)
      setPageLoc(curr_x_, curr_y_ - scroll_dist_);
    else if (key->code == sf::Keyboard::Key::K)
      setPageLoc(curr_x_, curr_y_ + scroll_dist_);
    else if (key->code == sf::Keyboard::Key::H)
      setPageLoc(curr_x_ + scroll_dist_, curr_y_);
    else if (key->code == sf::Keyboard::Key::L)
      setPageLoc(curr_x_ - scroll_dist_, curr_y_);
    else if (key->code == sf::Keyboard::Key::U)
      setPageLoc(curr_x_, curr_y_ + window_size_.y * 0.5f);
    else if (key->code == sf::Keyboard::Key::D)
      setPageLoc(curr_x_, curr_y_ - window_size_.y * 0.5f);
    else if (key->code == sf::Keyboard::Key::N)
      current_page_ = std::min(document_.size() - 1, current_page_ + 1);
    else if (key->code == sf::Keyboard::Key::P)
      current_page_ = std::max(0, (int)current_page_ - 1);
    else if (key->code == sf::Keyboard::Key::F) {
      const float width_zoom = visual_zoom_ * window_size_.x / sprite_.getGlobalBounds().size.x;
      const float height_zoom = visual_zoom_ * (window_size_.y - utils::cmdline_height_) /
                                sprite_.getGlobalBounds().size.y;
      setZoom(std::min(width_zoom, height_zoom));
    } else if (key->code == sf::Keyboard::Key::W)
      setZoom(visual_zoom_ * window_size_.x / sprite_.getGlobalBounds().size.x);
  }
}

void PDFView::onResize(const sf::Vector2f &size) {
  window_size_ = size;
  setPageLoc(curr_x_, curr_y_);
}

void PDFView::setZoom(float new_zoom) {
  if (!has_document_)
    return;

  const float clamped = std::clamp(new_zoom, min_zoom_, max_zoom_);
  if (clamped == visual_zoom_)
    return;

  event_bus_.emit("statusbar.page_zoom", clamped);
  visual_zoom_ = clamped;
  page_update_timer_.restart();
  setPageLoc(curr_x_, curr_y_);

  const auto bounds = sprite_.getGlobalBounds();
  update_scroll_bar_horizontal_ = bounds.size.x > window_size_.x;
  update_scroll_bar_vertical_ = bounds.size.y > window_size_.y;
}

void PDFView::setRotate(int rotate) {
  if (!has_document_)
    return;

  visual_rotate_ = (rotate % 4 + 4) % 4;
  page_update_timer_.restart();

  setPageLoc(curr_x_, curr_y_);

  update_scroll_bar_horizontal_ = true;
  update_scroll_bar_vertical_ = true;
}

float PDFView::map(float value, float src_min, float src_max, float dst_min, float dst_max) {
  float t = (value - src_min) / (src_max - src_min);
  return std::lerp(dst_min, dst_max, t);
}

void PDFView::getPage(std::size_t page_num, float zoom, int rotate) {
  texture_ = renderer_.render(page_num, zoom, rotate);
  texture_.setSmooth(false);
  sprite_.setTexture(texture_, true);
  sprite_.setScale({1.f, 1.f});
  centerPage();
}

void PDFView::setPageLoc(float x, float y) {
  const float scale = visual_zoom_ / render_zoom_;
  const int delta = (visual_rotate_ - render_rotate_ + 4) % 4;

  sprite_.setScale({scale, scale});
  sprite_.setRotation(sf::degrees(delta * 90.f));

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

  update_scroll_bar_horizontal_ = curr_x_ != x && page_w > window_size_.x;
  update_scroll_bar_vertical_ = curr_y_ != y && page_h > viewport_h;

  curr_x_ = x;
  curr_y_ = y;
}

void PDFView::centerPage() {
  const auto bounds = sprite_.getLocalBounds();
  sprite_.setOrigin({std::round(bounds.size.x / 2.f), std::round(bounds.size.y / 2.f)});
}

void PDFView::resetView() {
  has_document_ = false;

  current_page_ = 0;
  render_page_ = 0;

  visual_zoom_ = 1.f;
  render_zoom_ = 1.f;

  render_rotate_ = 0;
  visual_rotate_ = 0;

  setPageLoc(0.f, 0.f);
  texture_ = sf::Texture{};
  sprite_.setTexture(texture_, true);
}

} // namespace ui
