#include "ui/cursor.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

Cursor::Cursor(core::EventBus &event_bus, const std::string &typing_event) : event_bus_(event_bus) {
  cursor_.setSize({2.f, 24.f});
  cursor_.setFillColor(utils::hexToRGB(settings::fg_));

  const auto bounds = cursor_.getLocalBounds();
  cursor_.setOrigin(bounds.getCenter());

  event_bus_.subscribe<bool>(typing_event, [this](bool) {
    if (!blinking_)
      return;

    visible_ = true;
    clock_.restart();
  });
}

void Cursor::draw(sf::RenderTarget &window) const {
  if (visible_)
    window.draw(cursor_);
}

void Cursor::update() {
  if (!blinking_) {
    visible_ = false;
    return;
  }

  const auto elapsed = clock_.getElapsedTime();

  if (elapsed < typing_delay_) {
    visible_ = true;
    return;
  }

  const auto blink = elapsed - typing_delay_;
  visible_ = (blink.asMilliseconds() / blink_interval_.asMilliseconds()) % 2 == 0;
}

void Cursor::setPosition(const sf::Vector2f &position) {
  cursor_.setPosition(position);
}

void Cursor::start() {
  blinking_ = true;
  visible_ = true;
  clock_.restart();
}

void Cursor::stop() {
  blinking_ = false;
  visible_ = false;
}

} // namespace ui
