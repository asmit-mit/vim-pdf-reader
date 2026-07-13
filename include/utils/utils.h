#pragma once

#include <cstdint>

#include "SFML/Graphics.hpp"

namespace utils {

const float padding = 8.f;

inline sf::Color hexToRGB(const uint32_t hex) {
  return sf::Color((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
}

} // namespace utils
