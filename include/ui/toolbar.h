#pragma once

#include "core/event_bus.h"
#include "ui/widget.h"

namespace ui {

class Toolbar : public Widget {
public:
  explicit Toolbar(const sf::Font &font, core::EventBus &event_bus);

  void draw(sf::RenderTarget &window) const override;
  void update() override;
  void handleEvent(const sf::Event &event) override;

  void onResize(const sf::Vector2f &size);

private:
  std::string text_;
  const sf::Font &font_;
  bool cmdline_visible_;

  static constexpr float height_ = 24.0;

  float curr_x_, curr_y_;

  core::EventBus &event_bus_;

  sf::RectangleShape display_area_;
  sf::Text display_text_;
};

} // namespace ui
