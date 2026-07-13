#include "SFML/Window/Keyboard.hpp"

#include "ui/toolbar.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

Toolbar::Toolbar(const sf::Font &font, core::EventBus &event_bus)
    : font_(font), event_bus_(event_bus), display_text_(font_, "Nothing Open Yet", 16) {
  display_area_.setFillColor(utils::hexToRGB(settings::status_bg_));
  display_area_.setSize({200.0, height_});

  display_text_.setFillColor(utils::hexToRGB(settings::fg_));

  text_ = "Nothing Open Yet";
  cmdline_visible_ = false;

  curr_x_ = 0.f;
  curr_y_ = 0.f;

  event_bus_.subscribe<bool>("cmdline.visible", [this](bool visible) {
    cmdline_visible_ = visible;
  });
}

void Toolbar::draw(sf::RenderTarget &window) const {
  window.draw(display_area_);
  window.draw(display_text_);
}

void Toolbar::update() {
  display_text_.setString(text_);

  float y = curr_y_;

  if (cmdline_visible_)
    y -= height_;

  display_area_.setPosition({curr_x_, y});
  display_text_.setPosition({utils::padding, display_area_.getGlobalBounds().getCenter().y});
}

void Toolbar::handleEvent(const sf::Event &event) {}

void Toolbar::onResize(const sf::Vector2f &size) {
  curr_x_ = 0;
  curr_y_ = size.y - height_;

  display_area_.setSize({size.x, height_});

  const auto text_bounds = display_text_.getLocalBounds();
  display_text_.setOrigin({text_bounds.position.x, text_bounds.getCenter().y});
}

} // namespace ui
