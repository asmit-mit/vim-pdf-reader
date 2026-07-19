#include "ui/scrollwheel.h"

namespace ui {

ScrollWheel::ScrollWheel() {
  wheel_.setFillColor(sf::Color(255, 255, 255, 180));
}

void ScrollWheel::setSize(const sf::Vector2f &size) {
  wheel_.setSize(size);
  timer_.restart();
}

void ScrollWheel::setPosition(const sf::Vector2f &pos) {
  wheel_.setPosition(pos);
  visible_ = true;
  timer_.restart();
}

void ScrollWheel::update() {
  if (visible_ && timer_.getElapsedTime().asSeconds() >= delay_)
    visible_ = false;
}

void ScrollWheel::draw(sf::RenderTarget &window) const {
  if (!visible_)
    return;

  window.draw(wheel_);
}

} // namespace ui
