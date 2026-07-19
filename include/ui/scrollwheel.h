#pragma once

#include "core/event_bus.h"
#include "ui/widget.h"

#include <SFML/System/Clock.hpp>

namespace ui {

class ScrollWheel : public Widget {
public:
  explicit ScrollWheel(core::EventBus &event_bus);

  void draw(sf::RenderTarget &window) const override;
  void update() override;
  void handleEvent(const sf::Event &event) override {}

  void setSize(const sf::Vector2f &size);
  void setPosition(const sf::Vector2f &pos);

private:
  core::EventBus &event_bus_;

  sf::RectangleShape wheel_;

  bool visible_ = false;
  sf::Clock timer_;

  static constexpr float delay_ = 1.f;
};

} // namespace ui
