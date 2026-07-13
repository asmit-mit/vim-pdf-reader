#pragma once

#include <cstdint>

#include "SFML/Graphics.hpp"
#include "ui/cmdline.h"

class App {
public:
  App();

  void run();

private:
  void processEvents();

private:
  sf::RenderWindow window_;
  sf::View view_;
  sf::Font font_;

  ui::Cmdline cmdline;

  const unsigned int res_x_ = 640;
  const unsigned int res_y_ = 480;
  const unsigned int fps_ = 60;
};
