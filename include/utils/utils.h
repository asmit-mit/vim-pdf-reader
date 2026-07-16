#pragma once

#include <cstdint>
#include <filesystem>

#include "SFML/Graphics.hpp"

namespace utils {

const float padding = 8.f;
const float cmdline_height_ = 24.f;

inline sf::Color hexToRGB(const uint32_t hex) {
  return sf::Color((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
}

inline std::string resolvePath(std::string path) {
  if (path.starts_with("~/")) {
    if (const char *home = std::getenv("HOME"))
      path = (std::filesystem::path(home) / path.substr(2)).string();
  }

  return path;
}

inline bool isNumber(const std::string &s) {
  return !s.empty() &&
         std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}

} // namespace utils
