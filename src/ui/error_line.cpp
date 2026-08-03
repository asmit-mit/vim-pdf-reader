#include "ui/error_line.h"
#include "ui/ui_elements.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

ErrorLine::ErrorLine(const sf::Font &font_normal, core::EventBus &event_bus)
    : event_bus_(event_bus), font_(font_normal), textbox_(font_, ":", utils::char_size) {
  visible_ = false;

  textbox_.setFillColor(utils::hexToRGB(settings::fg_));
  display_area_.setFillColor(utils::hexToRGB(settings::cmd_bg_));
  display_area_.setSize({200.0, utils::cmdline_height_});

  event_bus_.subscribe<ui::UIElements>("ui.focus", [this](ui::UIElements focus) {
    if (focus == ui::UIElements::ErrorLine && !visible_)
      event_bus_.emit("ui.focus", ui::UIElements::PDFView);
  });

  event_bus_.subscribe<const char *>("cmdline.msg", [this](const char *msg) {
    visible_ = true;
    textbox_.setString(msg);
  });
}

void ErrorLine::draw(sf::RenderTarget &window) const {
  if (!visible_)
    return;

  window.draw(display_area_);
  window.draw(textbox_);
}

void ErrorLine::handleEvent(const sf::Event &event) {
  if (!visible_)
    return;

  const auto *key = event.getIf<sf::Event::KeyPressed>();
  if (key) {
    if (key->code == sf::Keyboard::Key::Escape) {
      visible_ = false;
      event_bus_.emit("ui.focus", ui::UIElements::PDFView);
    } else if (key->code == sf::Keyboard::Key::Semicolon && key->shift) {
      visible_ = false;
      event_bus_.emit("ui.focus", ui::UIElements::Cmdline);
    }
  }
}

void ErrorLine::onResize(const sf::Vector2f &size) {
  window_size_ = size;
  display_area_.setSize({size.x, utils::cmdline_height_});

  const float curr_x_ = 0.f;
  const float curr_y_ = window_size_.y - utils::cmdline_height_;
  float round_y = std::round(curr_y_) + 1.f;

  display_area_.setPosition({curr_x_, round_y});
  textbox_.setPosition({utils::padding, round_y});
}

} // namespace ui
