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

  void setFillColor(const sf::Color &color);
  void setSize(const sf::Vector2f &size);
  void setPosition(const sf::Vector2f &pos);

private:
  sf::RectangleShape wheel_;
  sf::Clock timer_;
  sf::Color base_color_;

  bool visible_ = false;

  static constexpr float delay_ = 1.f;
  static constexpr float fade_duration_ = 0.25f;
};

} // namespace ui
