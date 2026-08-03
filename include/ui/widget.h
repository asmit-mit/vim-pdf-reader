#pragma once

#include "SFML/Graphics.hpp"

class Widget {
public:
  virtual ~Widget() = default;

  virtual void handleEvent(const sf::Event &) {};
  virtual void update() {};
  virtual void draw(sf::RenderTarget &) const = 0;
};
