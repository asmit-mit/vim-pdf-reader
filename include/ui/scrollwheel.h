#pragma once

#include "ui/widget.h"

#include <SFML/Graphics.hpp>

namespace ui {

class ScrollWheel : public Widget {
public:
  explicit ScrollWheel();

  void draw(sf::RenderTarget &window) const override;
  void update() override;
  void handleEvent(const sf::Event &event) override {}

  void setSize(const sf::Vector2f &size);
  void setPosition(const sf::Vector2f &pos);

private:
  sf::RectangleShape wheel_;

  bool visible_ = false;
  sf::Clock timer_;

  static constexpr float delay_ = 1.f;
};

} // namespace ui
