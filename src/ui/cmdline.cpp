#include "ui/cmdline.h"
#include "utils/settings.h"
#include "utils/utils.h"
#include <SFML/Window/Keyboard.hpp>

namespace ui {

Cmdline::Cmdline(const sf::Font &font) : font_(font), display_text_(font_, ":", 16) {
  is_enabled_ = false;
  cursor_visible_ = false;

  display_area_.setFillColor(utils::hexToRGB(settings::cmd_bg_));
  display_area_.setSize({200.0, height_});

  cursor_.setFillColor(utils::hexToRGB(settings::fg_));
  cursor_.setSize({2, height_});

  display_text_.setFillColor(utils::hexToRGB(settings::fg_));
}

void Cmdline::draw(sf::RenderTarget &window) const {
  if (!is_enabled_)
    return;

  window.draw(display_area_);
  window.draw(display_text_);

  if (cursor_visible_)
    window.draw(cursor_);
}

void Cmdline::update() {
  if (!is_enabled_)
    return;

  display_text_.setString(text_ + " ");

  const auto text_bounds = display_text_.getLocalBounds();
  cursor_.setOrigin({
      text_bounds.position.x,
      text_bounds.position.y + text_bounds.size.y / 2.f,
  });
  cursor_.setPosition({
      text_bounds.position.x + text_bounds.size.x,
      display_area_.getGlobalBounds().getCenter().y,
  });

  const auto elapsed = cursor_clock_.getElapsedTime();
  if (elapsed < typing_delay_) {
    cursor_visible_ = true;
  } else {
    const auto blinkTime = elapsed - typing_delay_;
    cursor_visible_ = (blinkTime.asMilliseconds() / blink_interval_.asMilliseconds()) % 2 == 0;
  }
}

void Cmdline::handleEvent(const sf::Event &event) {
  if (const auto *key = event.getIf<sf::Event::KeyPressed>()) {
    if (!is_enabled_) {
      if (key->code == sf::Keyboard::Key::Semicolon && key->shift) {
        is_enabled_ = true;
        text_.clear();
        cursor_clock_.restart();
      }
      return;
    }

    if (key->code == sf::Keyboard::Key::Escape) {
      is_enabled_ = false;
      cursor_visible_ = false;
    } else if (key->code == sf::Keyboard::Key::Backspace && key->control) {
      handleSpecialBackspace();
      cursor_clock_.restart();
    } else if (key->code == sf::Keyboard::Key::Backspace) {
      if (text_.size() > 1) {
        text_.pop_back();
        cursor_clock_.restart();
      }
    }
  }

  if (!is_enabled_)
    return;

  if (const auto *text = event.getIf<sf::Event::TextEntered>()) {
    if (text->unicode >= 32 && text->unicode < 127) {
      text_.push_back(static_cast<char>(text->unicode));
      cursor_clock_.restart();
    }
  }
}

void Cmdline::onResize(const sf::Vector2f &size) {
  display_area_.setSize({size.x, height_});
  display_area_.setPosition({0.0, size.y - height_});

  const auto text_bounds = display_text_.getLocalBounds();
  display_text_.setOrigin(
      {text_bounds.position.x, text_bounds.position.y + text_bounds.size.y / 2.f}
  );
  display_text_.setPosition({utils::padding, display_area_.getGlobalBounds().getCenter().y});
}

void Cmdline::handleSpecialBackspace() {
  while (text_.size() > 1 && std::isspace(static_cast<unsigned char>(text_.back())))
    text_.pop_back();
  while (text_.size() > 1 && !std::isalnum(static_cast<unsigned char>(text_.back())) &&
         text_.back() != '_' && !std::isspace(static_cast<unsigned char>(text_.back())))
    text_.pop_back();
  while (text_.size() > 1 &&
         (std::isalnum(static_cast<unsigned char>(text_.back())) || text_.back() == '_'))
    text_.pop_back();
}

} // namespace ui
