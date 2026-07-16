#include "SFML/Window/Keyboard.hpp"

#include "ui/statusbar.h"
#include "ui/ui_elements.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

Statusbar::Statusbar(const sf::Font &font, core::EventBus &event_bus)
    : font_(font), display_filepath_(font_, "[Nothing Open Yet]", 16),
      display_page_num_(font_, "[]", 16), display_zoom_(font_, "[]", 16), event_bus_(event_bus) {
  display_area_.setFillColor(utils::hexToRGB(settings::status_bg_));
  display_area_.setSize({200.0, height_});

  display_filepath_.setFillColor(utils::hexToRGB(settings::fg_));
  display_page_num_.setFillColor(utils::hexToRGB(settings::fg_));
  display_zoom_.setFillColor(utils::hexToRGB(settings::fg_));

  page_idx_ = 0;
  total_pages_ = 0;
  cmdline_visible_ = false;
  page_details_changed_ = false;

  curr_x_ = 0.f;
  curr_y_ = 0.f;

  event_bus_.subscribe<ui::UIElements>("ui.focus", [this](ui::UIElements focus) {
    cmdline_visible_ = focus == ui::UIElements::Cmdline;
  });

  event_bus_.subscribe<std::string>("toolbar.pdf_path", [this](const std::string &filepath) {
    display_filepath_.setString(filepath);
  });

  event_bus_.subscribe<std::size_t>("toolbar.page_number", [this](std::size_t page_number) {
    page_idx_ = page_number;
    page_details_changed_ = true;
  });

  event_bus_.subscribe<std::size_t>("toolbar.total_pages", [this](std::size_t total_pages) {
    total_pages_ = total_pages;
    page_details_changed_ = true;
  });

  event_bus_.subscribe<float>("toolbar.page_zoom", [this](float zoom) {
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
        "[" + std::to_string(page_idx_ + 1) + "/" + std::to_string(total_pages_) + "]"
    );
    page_details_changed_ = false;
  }

  float y = std::round(curr_y_);

  if (cmdline_visible_)
    y -= height_;

  display_area_.setPosition({curr_x_, y});

  const float round_y = display_area_.getGlobalBounds().getCenter().y;
  display_filepath_.setPosition({utils::padding, round_y});

  const float page_num_x = std::round(
      display_area_.getSize().x - display_page_num_.getLocalBounds().size.x - utils::padding
  );
  const float zoom_x = std::round(
      page_num_x - display_zoom_.getLocalBounds().size.x - utils::padding
  );

  display_page_num_.setPosition({page_num_x, round_y});
  display_zoom_.setPosition({zoom_x, round_y});
}

void Statusbar::handleEvent(const sf::Event &event) {}

void Statusbar::onResize(const sf::Vector2f &size) {
  curr_x_ = 0;
  curr_y_ = size.y - height_;

  display_area_.setSize({size.x, height_});

  const auto filepath_bounds = display_filepath_.getLocalBounds();
  display_filepath_.setOrigin({filepath_bounds.position.x, filepath_bounds.getCenter().y});

  const auto page_num_bounds = display_page_num_.getLocalBounds();
  display_page_num_.setOrigin({page_num_bounds.position.x, page_num_bounds.getCenter().y});

  const auto zoom_bounds = display_zoom_.getLocalBounds();
  display_zoom_.setOrigin({zoom_bounds.position.x, zoom_bounds.getCenter().y});
}

} // namespace ui
