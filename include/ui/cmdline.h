#pragma once

#include "core/event_bus.h"
#include "ui/cursor.h"
#include "widget.h"

namespace ui {

enum class CmdlineState { Status, Edit, Hidden };

class Cmdline : public Widget {
public:
  explicit Cmdline(const sf::Font &font, core::EventBus &event_bus);

  void draw(sf::RenderTarget &window) const override;
  void update() override;
  void handleEvent(const sf::Event &event) override;

  void onResize(const sf::Vector2f &size);

private:
  void handleSpecialBackspace();

private:
  core::EventBus &event_bus_;

  std::string text_;
  const sf::Font &font_;
  CmdlineState state_;

  static constexpr float height_ = 24.0;

  float curr_x_, curr_y_;

  ui::Cursor cursor_;
  sf::RectangleShape display_area_;
  sf::Text display_text_;

};

} // namespace ui
