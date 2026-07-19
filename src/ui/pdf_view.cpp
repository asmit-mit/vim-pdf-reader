#include <iostream>
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
  update_scroll_bar_ = false;

  event_bus_
      .subscribe<std::string>("cmd_processor.open_document", [this](const std::string &filepath) {
        try {
          document_.openDocument(utils::resolvePath(filepath));
          resetView();
          has_document_ = true;

          getPage(current_page_, render_zoom_);
          setPageLoc(window_size_.x / 2.f, window_size_.y / 2.f);

          event_bus_.emit("statusbar.pdf_path", filepath);
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
}

void PDFView::update() {
  if (!has_document_)
    return;

  const float scale = visual_zoom_ / render_zoom_;
  sprite_.setScale({scale, scale});

  if ((render_zoom_ != visual_zoom_) &&
      zoom_timer_.getElapsedTime().asMilliseconds() > zoom_dobounce_ms_) {
    getPage(current_page_, visual_zoom_);
    render_zoom_ = visual_zoom_;
  }

  if (render_page_ != current_page_) {
    getPage(current_page_, visual_zoom_);
    render_page_ = current_page_;
    event_bus_.emit("statusbar.page_number", current_page_);
  }

  if (update_scroll_bar_) {
    std::cout << "loc changed" << std::endl;
    update_scroll_bar_ = false;
  }

  sprite_.setPosition({std::round(curr_x_), std::round(curr_y_)});
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
    else if (key->code == sf::Keyboard::Key::G) {
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
  zoom_timer_.restart();
  setPageLoc(curr_x_, curr_y_);
  update_scroll_bar_ = true;
}

void PDFView::getPage(std::size_t page_num, float zoom) {
  texture_ = renderer_.render(page_num, zoom);
  sprite_.setTexture(texture_, true);
  sprite_.setScale({1.f, 1.f});
  centerPage();
}

void PDFView::setPageLoc(float x, float y) {
  const float scale = visual_zoom_ / render_zoom_;

  const auto bounds = sprite_.getLocalBounds();
  const float page_w = bounds.size.x * scale;
  const float page_h = bounds.size.y * scale;

  if (page_w <= window_size_.x)
    x = window_size_.x / 2.f;
  else
    x = std::clamp(x, window_size_.x - page_w / 2.f, page_w / 2.f);

  if (page_h <= window_size_.y)
    y = window_size_.y / 2.f;
  else
    y = std::clamp(y, window_size_.y - (page_h / 2.f) - utils::cmdline_height_, page_h / 2.f);

  if (curr_x_ != x || curr_y_ != y)
    update_scroll_bar_ = true;

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

  setPageLoc(0.f, 0.f);

  texture_ = sf::Texture{};
  sprite_.setTexture(texture_, true);
}

} // namespace ui
