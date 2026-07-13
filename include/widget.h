#pragma once

#include "SFML/Graphics.hpp"

class Widget {
public:
  virtual ~Widget() = default;

  virtual void handleEvent(const sf::Event &) = 0;
  virtual void update() = 0;
  virtual void draw(sf::RenderTarget &) const = 0;
};
