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

  event_bus_
      .subscribe<std::string>("cmd_processor.open_document", [this](const std::string &filepath) {
        try {
          document_.openDocument(utils::resolvePath(filepath));
          has_document_ = true;
          current_page_ = 0;
          render_page_ = 0;
          visual_zoom_ = 1.f;
          render_zoom_ = 1.f;

          getPage(current_page_, render_zoom_);

          curr_x_ = window_size_.x / 2.f;
          curr_y_ = window_size_.y / 2.f;

          event_bus_.emit("toolbar.pdf_path", filepath);
          event_bus_.emit("toolbar.page_number", 0);
          event_bus_.emit("toolbar.total_pages", document_.size());
          event_bus_.emit("toolbar.page_zoom", visual_zoom_);
        } catch (const std::runtime_error &e) {
          has_document_ = false;
          event_bus_.emit("cmdline.msg", e.what());
        }
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

void PDFView::setZoom(float new_zoom) {
  if (!has_document_)
    return;

  const float clamped = std::clamp(new_zoom, min_zoom_, max_zoom_);
  if (clamped == visual_zoom_)
    return;

  event_bus_.emit("toolbar.page_zoom", visual_zoom_);
  visual_zoom_ = clamped;
  zoom_timer_.restart();
}

void PDFView::getPage(std::size_t page_num, float zoom) {
  texture_ = renderer_.render(page_num, zoom);
  sprite_.setTexture(texture_, true);
  sprite_.setScale({1.f, 1.f});
  centerPage();
}

void PDFView::centerPage() {
  const auto bounds = sprite_.getLocalBounds();
  sprite_.setOrigin({std::round(bounds.size.x / 2.f), std::round(bounds.size.y / 2.f)});
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
    event_bus_.emit("toolbar.page_number", current_page_);
  }

  const auto bounds = sprite_.getGlobalBounds();
  const float page_w = bounds.size.x;
  const float page_h = bounds.size.y;

  if (page_w <= window_size_.x)
    curr_x_ = window_size_.x / 2.f;
  else
    curr_x_ = std::clamp(curr_x_, window_size_.x - page_w / 2.f, page_w / 2.f);

  if (page_h <= window_size_.y)
    curr_y_ = window_size_.y / 2.f;
  else
    curr_y_ = std::clamp(curr_y_, window_size_.y - page_h / 2.f, page_h / 2.f);

  sprite_.setPosition({std::round(curr_x_), std::round(curr_y_)});
}

void PDFView::handleEvent(const sf::Event &event) {
  if (!has_document_)
    return;
  const auto *key = event.getIf<sf::Event::KeyPressed>();
  if (key && should_take_input_) {
    if (key->code == sf::Keyboard::Key::Equal && key->shift)
      setZoom(visual_zoom_ + settings::delta_zoom_);
    else if (key->code == sf::Keyboard::Key::Hyphen && key->shift)
      setZoom(visual_zoom_ - settings::delta_zoom_);
    else if (key->code == sf::Keyboard::Key::Equal)
      setZoom(1.f);
    else if (key->code == sf::Keyboard::Key::J)
      curr_y_ -= scroll_dist_;
    else if (key->code == sf::Keyboard::Key::K)
      curr_y_ += scroll_dist_;
    else if (key->code == sf::Keyboard::Key::H)
      curr_x_ += scroll_dist_;
    else if (key->code == sf::Keyboard::Key::L)
      curr_x_ -= scroll_dist_;
    else if (key->code == sf::Keyboard::Key::U)
      curr_y_ += window_size_.y * 0.5f;
    else if (key->code == sf::Keyboard::Key::D)
      curr_y_ -= window_size_.y * 0.5f;
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
}

} // namespace ui
