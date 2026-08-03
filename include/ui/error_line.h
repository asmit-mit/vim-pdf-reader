#pragma once

#include "core/event_bus.h"
#include "ui/widget.h"

namespace ui {

class ErrorLine : public Widget {
public:
  explicit ErrorLine(const sf::Font &font_normal, core::EventBus &event_bus);

  void draw(sf::RenderTarget &window) const override;
  void handleEvent(const sf::Event &event) override;

  void onResize(const sf::Vector2f &size);

private:
  core::EventBus &event_bus_;
  const sf::Font &font_;

  sf::Text textbox_;
  sf::RectangleShape display_area_;

  sf::Vector2f window_size_;

  bool visible_;
};

} // namespace ui
