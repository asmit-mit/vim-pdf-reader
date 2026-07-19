#include "ui/scrollwheel.h"

namespace ui {

ScrollWheel::ScrollWheel() {}

void ScrollWheel::setFillColor(const sf::Color &color) {
  base_color_ = color;
  wheel_.setFillColor(color);
}

void ScrollWheel::setSize(const sf::Vector2f &size) {
  wheel_.setSize(size);
  timer_.restart();
}

void ScrollWheel::setPosition(const sf::Vector2f &pos) {
  wheel_.setPosition(pos);
  visible_ = true;
  timer_.restart();

  wheel_.setFillColor(base_color_);
}

void ScrollWheel::update() {
  float elapsed = timer_.getElapsedTime().asSeconds();
  if (elapsed < delay_) {
    visible_ = true;
    wheel_.setFillColor(base_color_);
    return;
  }

  float t = (elapsed - delay_) / fade_duration_;
  if (t >= 1.f) {
    visible_ = false;
    return;
  }

  visible_ = true;

  sf::Color c = base_color_;
  c.a = static_cast<std::uint8_t>(255.f * (1.f - t));
  wheel_.setFillColor(c);
}

void ScrollWheel::draw(sf::RenderTarget &window) const {
  if (!visible_)
    return;
  window.draw(wheel_);
}

} // namespace ui
