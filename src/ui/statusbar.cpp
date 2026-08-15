#include "ui/statusbar.h"
#include "ui/ui_elements.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

Statusbar::Statusbar(
    graphics::FontLibrary &font_lib,
    graphics::GlyphAtlas &glyph_atlas,
    const sf::Font &font,
    core::EventBus &event_bus
)
    : font_(font), display_filepath_(font_lib, glyph_atlas, 16), display_page_num_(font_, "[]", 16),
      display_zoom_(font_, "[]", 16), event_bus_(event_bus) {
  display_area_.setFillColor(utils::hexToRGB(settings::status_bg_));
  display_area_.setSize({200.0, height_});

  display_filepath_.setFillColor(utils::hexToRGB(settings::fg_));
  display_page_num_.setFillColor(utils::hexToRGB(settings::fg_));
  display_zoom_.setFillColor(utils::hexToRGB(settings::fg_));

  display_filepath_.setString("[Nothing Open Yet]");

  page_idx_ = 0;
  total_pages_ = 0;
  move_up_ = false;
  page_details_changed_ = false;

  curr_x_ = 0.f;
  curr_y_ = 0.f;

  event_bus_.subscribe<ui::UIElements>("ui.focus", [this](ui::UIElements focus) {
    move_up_ = focus == ui::UIElements::Cmdline;
  });

  event_bus_.subscribe<std::string>("statusbar.pdf_path", [this](const std::string &filepath) {
    display_filepath_.setString(filepath);
  });

  event_bus_.subscribe<std::size_t>("statusbar.page_number", [this](std::size_t page_number) {
    page_idx_ = page_number;
    page_details_changed_ = true;
  });

  event_bus_.subscribe<std::size_t>("statusbar.total_pages", [this](std::size_t total_pages) {
    total_pages_ = total_pages;
    page_details_changed_ = true;
  });

  event_bus_.subscribe<float>("statusbar.page_zoom", [this](float zoom) {
    display_zoom_.setString("[" + std::to_string((int)(zoom * 100)) + "%]");
  });
}

void Statusbar::draw(sf::RenderTarget &window) const {
  window.draw(display_area_);
  window.draw(display_filepath_);
  if (total_pages_ != 0) {
    window.draw(display_page_num_);
    window.draw(display_zoom_);
  }
}

void Statusbar::update() {
  if (page_details_changed_) {
    display_page_num_.setString(
        "[" + std::to_string(page_idx_) + "/" + std::to_string(total_pages_) + "]"
    );
    page_details_changed_ = false;
  }

  float y = std::round(curr_y_);
  if (move_up_)
    y = (y - height_) + 1.f;

  display_area_.setPosition({curr_x_, y});

  const float filepath_h = display_filepath_.getSize().y;
  const float filepath_y = std::round(y + (height_ - filepath_h) / 2.f);

  display_filepath_.setPosition({utils::padding, filepath_y - 3.f});

  const float text_y_base = y + height_ / 2.f;

  const auto page_num_bounds = display_page_num_.getLocalBounds();
  const float page_num_y = std::round(
      text_y_base - page_num_bounds.size.y / 2.f - page_num_bounds.position.y
  );
  const float page_num_x = std::round(
      display_area_.getSize().x - page_num_bounds.size.x - utils::padding
  );

  const auto zoom_bounds = display_zoom_.getLocalBounds();
  const float zoom_y = std::round(text_y_base - zoom_bounds.size.y / 2.f - zoom_bounds.position.y);
  const float zoom_x = std::round(page_num_x - zoom_bounds.size.x - utils::padding);

  display_page_num_.setPosition({page_num_x, page_num_y});
  display_zoom_.setPosition({zoom_x, zoom_y});
}

void Statusbar::onResize(const sf::Vector2f &size) {
  curr_x_ = 0;
  curr_y_ = size.y - height_;
  display_area_.setSize({size.x, height_});
}

} // namespace ui
