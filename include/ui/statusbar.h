#pragma once

#include "core/event_bus.h"
#include "graphics/rich_text.h"
#include "ui/widget.h"

namespace ui {

class Statusbar : public Widget {
public:
  explicit Statusbar(
      graphics::FontLibrary &font_lib,
      graphics::GlyphAtlas &glyph_atlas,
      const sf::Font &font,
      core::EventBus &event_bus
  );

  void draw(sf::RenderTarget &window) const override;
  void update() override;

  void onResize(const sf::Vector2f &size);

private:
  const sf::Font &font_;
  sf::RectangleShape display_area_;
  graphics::RichText display_filepath_;
  sf::Text display_page_num_;
  sf::Text display_zoom_;

  core::EventBus &event_bus_;

  std::size_t page_idx_;
  std::size_t total_pages_;
  bool page_details_changed_;
  bool move_up_;
  float curr_x_, curr_y_;

  static constexpr float height_ = 24.0;
};

} // namespace ui
