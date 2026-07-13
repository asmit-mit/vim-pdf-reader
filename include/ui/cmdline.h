#pragma once

#include "widget.h"

namespace ui {

class Cmdline : public Widget {
public:
  explicit Cmdline(const sf::Font &font);

  void draw(sf::RenderTarget &window) const override;
  void update() override;
  void handleEvent(const sf::Event &event) override;

  void onResize(const sf::Vector2f &size);

private:
  void handleSpecialBackspace();

private:
  bool is_enabled_;
  bool cursor_visible_;
  std::string text_;
  const sf::Font &font_;

  static constexpr float height_ = 24.0;

  sf::RectangleShape cursor_;
  sf::RectangleShape display_area_;
  sf::Text display_text_;
  sf::Clock cursor_clock_;

  static constexpr auto typing_delay_ = sf::milliseconds(500);
  static constexpr auto blink_interval_ = sf::milliseconds(500);
};

} // namespace ui
