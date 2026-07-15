#include <iostream>
#include <stdexcept>

#include "ui/pdf_view.h"
#include "ui/ui_elements.h"
#include "utils/utils.h"

namespace ui {

PDFView::PDFView(core::EventBus &event_bus)
    : renderer_(document_), event_bus_(event_bus), sprite_(sf::Sprite(texture_)) {
  curr_x_ = 0.f;
  curr_y_ = 0.f;

  event_bus_
      .subscribe<std::string>("cmd_processor.open_document", [this](const std::string &filepath) {
        try {
          document_.openDocument(utils::resolvePath(filepath));
          has_document_ = true;
          current_page_ = 0;
          zoom_ = 1.f;

          texture_ = renderer_.render(current_page_, zoom_);
          sprite_.setTexture(texture_, true);

        } catch (const std::runtime_error &e) {
          has_document_ = false;
          event_bus_.emit("status.msg", std::string(e.what()));
        }
      });

  event_bus_.subscribe<ui::UIElements>("ui.focus", [this](ui::UIElements focus) {
    should_take_input_ = focus == ui::UIElements::PDFView;
  });
}

void PDFView::setZoom(float zoom) {
  zoom_ = std::clamp(zoom, min_zoom_, max_zoom_);

  if (!has_document_)
    return;

  texture_ = renderer_.render(current_page_, zoom_);
  sprite_.setTexture(texture_, true);
  std::cout << texture_.getSize().x << " " << texture_.getSize().y << std::endl;
}

void PDFView::draw(sf::RenderTarget &window) const {
  if (!has_document_)
    return;
  window.draw(sprite_);
}

void PDFView::update() {
  if (!has_document_)
    return;

  if (zoom_changed_) {
    setZoom(zoom_);
    zoom_changed_ = false;
  }

  const auto bounds = sprite_.getLocalBounds();
  sprite_.setOrigin({bounds.getCenter().x, bounds.position.y});

  curr_x_ = page_size_.x / 2.f;
  if (bounds.size.y < page_size_.y)
    curr_y_ = (page_size_.y - bounds.size.y) / 2.f;

  sprite_.setPosition({curr_x_, curr_y_});

  event_bus_.emit("toolbar.page_number", current_page_);
  event_bus_.emit("toolbar.page_zoom", zoom_);
}

void PDFView::handleEvent(const sf::Event &event) {
  if (!has_document_)
    return;

  const auto *key = event.getIf<sf::Event::KeyPressed>();

  if (key && should_take_input_) {
    if (key->code == sf::Keyboard::Key::Add || key->code == sf::Keyboard::Key::Equal) {
      zoom_ += 0.2f;
      zoom_changed_ = true;
    } else if (key->code == sf::Keyboard::Key::Subtract || key->code == sf::Keyboard::Key::Hyphen) {
      zoom_ -= 0.2f;
      zoom_changed_ = true;
    }
  }
}

void PDFView::onResize(const sf::Vector2f &size) {
  page_size_ = size;
}

} // namespace ui
