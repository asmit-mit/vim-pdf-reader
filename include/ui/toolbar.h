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
  const sf::Font &font_;
  sf::RectangleShape display_area_;
  sf::Text display_filepath_;
  sf::Text display_page_num_;
  sf::Text display_zoom_;

  core::EventBus &event_bus_;

  std::string filepath_;
  std::size_t page_idx_;
  std::size_t total_pages_;
  float page_zoom_;
  bool cmdline_visible_;
  float curr_x_, curr_y_;

  static constexpr float height_ = 24.0;
};

} // namespace ui
