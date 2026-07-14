#include <iostream>

#include "ui/cmdline.h"
#include "ui/cursor.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

Cmdline::Cmdline(
    const sf::Font &font,
    core::EventBus &event_bus,
    core::CmdProcessor &cmd_processor,
    core::CmdHistory &cmd_history
)
    : event_bus_(event_bus), cmd_processor_(cmd_processor), cmd_history_(cmd_history), font_(font),
      display_text_(font_, ":", 16), cursor_(event_bus_, "cmdline.typing") {
  state_ = CmdlineState::Hidden;

  curr_x_ = 0.f;
  curr_y_ = 0.f;

  display_area_.setFillColor(utils::hexToRGB(settings::cmd_bg_));
  display_area_.setSize({200.0, height_});

  display_text_.setFillColor(utils::hexToRGB(settings::fg_));

  event_bus_.subscribe<std::string>("status.msg", [this](const std::string &msg) {
    text_ = msg;
    state_ = CmdlineState::Status;
  });
}

void Cmdline::draw(sf::RenderTarget &window) const {
  if (state_ == CmdlineState::Hidden)
    return;

  window.draw(display_area_);
  window.draw(display_text_);
  cursor_.draw(window);
}

void Cmdline::update() {
  if (state_ == CmdlineState::Hidden) {
    event_bus_.emit("cmdline.visible", false);
    return;
  }

  event_bus_.emit("cmdline.visible", true);

  display_area_.setPosition({curr_x_, curr_y_});

  const auto &display_bounds = display_area_.getGlobalBounds();
  display_text_.setPosition({utils::padding, display_bounds.getCenter().y});
  display_text_.setString(text_);

  cursor_.update();

  sf::Vector2f local{0.f, 0.f};
  const auto &glyphs = display_text_.getShapedGlyphs();

  if (!glyphs.empty()) {
    const auto &last = glyphs.back();
    local = last.position;
    local.x += last.glyph.advance;
  }

  cursor_.setPosition(
      {display_text_.getTransform().transformPoint(local).x, display_bounds.getCenter().y}
  );
}

void Cmdline::handleEvent(const sf::Event &event) {
  const auto *key = event.getIf<sf::Event::KeyPressed>();
  const auto *text = event.getIf<sf::Event::TextEntered>();

  if (key) {
    if (state_ == CmdlineState::Hidden) {
      cursor_.stop();

      if (key->code == sf::Keyboard::Key::Semicolon && key->shift) {
        state_ = CmdlineState::Edit;
        text_.clear();
        cursor_.start();
      }

      return;
    }

    if (state_ == CmdlineState::Status) {
      cursor_.stop();

      if (key->code == sf::Keyboard::Key::Semicolon && key->shift) {
        state_ = CmdlineState::Edit;
        text_.clear();
        cursor_.start();
      } else if (key->code == sf::Keyboard::Key::Escape)
        state_ = CmdlineState::Hidden;

      return;
    }

    if (state_ == CmdlineState::Edit) {
      cursor_.start();

      if (key->code == sf::Keyboard::Key::Escape) {
        state_ = CmdlineState::Hidden;
        cmd_history_.reset();
        cursor_.stop();
      } else if (key->code == sf::Keyboard::Key::Enter) {
        if (text_.size() <= 1)
          return;
        state_ = CmdlineState::Hidden;
        cmd_history_.add(text_);
        cmd_processor_.runCommand(text_);
        cursor_.stop();
      } else if (key->code == sf::Keyboard::Key::Backspace && key->control) {
        handleSpecialBackspace();
      } else if (key->code == sf::Keyboard::Key::Backspace) {
        if (text_.size() > 1)
          text_.pop_back();
      } else if (key->code == sf::Keyboard::Key::V && key->control) {
        text_ += sf::Clipboard::getString().toAnsiString();
      } else if (key->code == sf::Keyboard::Key::Up)
        text_ = cmd_history_.getPrevious();
      else if (key->code == sf::Keyboard::Key::Down)
        text_ = cmd_history_.getNext();

      return;
    }
  }

  if (text && state_ == CmdlineState::Edit) {
    if (text->unicode >= 32 && text->unicode < 127) {
      text_.push_back(static_cast<char>(text->unicode));
      event_bus_.emit("cmdline.typing", true);
    }
    return;
  }
}

void Cmdline::onResize(const sf::Vector2f &size) {
  curr_x_ = 0.f;
  curr_y_ = size.y - height_;

  display_area_.setSize({size.x, height_});

  const auto text_bounds = display_text_.getLocalBounds();
  display_text_.setOrigin({text_bounds.position.x, text_bounds.getCenter().y});
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
