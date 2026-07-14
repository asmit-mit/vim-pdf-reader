#include "ui/textbox.h"
#include "utils/settings.h"
#include "utils/utils.h"
#include <SFML/Window/Keyboard.hpp>

namespace ui {

Textbox::Textbox(
    const sf::Font &font,
    core::EventBus &event_bus,
    int char_size,
    const std::string &input_string,
    const std::string &enable_event,
    const std::string &typing_event
)
    : event_bus_(event_bus), font_(font), display_text_(font, input_string, char_size),
      cursor_(event_bus, typing_event), cursor_event_(typing_event) {
  visible_ = false;
  editing_ = false;

  display_text_.setFillColor(utils::hexToRGB(settings::fg_));

  cursor_size_ = {0.f, 0.f};
  pos_ = {0.f, 0.f};
  cursor_pos_ = 0;

  event_bus_.subscribe<bool>(enable_event, [this](bool visible) { visible_ = visible; });
}

void Textbox::draw(sf::RenderTarget &window) const {
  if (!visible_)
    return;

  window.draw(display_text_);
  cursor_.draw(window);
}

void Textbox::update() {
  if (!visible_)
    return;

  display_text_.setString(text_);
  display_text_.setPosition(pos_);
  cursor_.update();

  if (cursor_pos_ == 0) {
    cursor_.setPosition({pos_});
    return;
  }

  const auto &glyphs = display_text_.getShapedGlyphs();

  sf::Vector2f local{0.f, 0.f};
  if (!glyphs.empty()) {
    if (cursor_pos_ < glyphs.size()) {
      local = glyphs[cursor_pos_].position;
    } else {
      const auto &last = glyphs.back();
      local = {last.position.x + last.glyph.advance, last.position.y};
    }
  }

  sf::Vector2f world = display_text_.getTransform().transformPoint(local);
  cursor_.setPosition({world.x, pos_.y});
}

void Textbox::handleEvent(const sf::Event &event) {
  if (!visible_ || !editing_)
    return;

  const auto *key = event.getIf<sf::Event::KeyPressed>();
  const auto *text = event.getIf<sf::Event::TextEntered>();

  if (key) {
    if (key->code == sf::Keyboard::Key::Backspace && key->control) {
      ctrlBackspace();
    } else if (key->code == sf::Keyboard::Key::Backspace) {
      backspace();
    } else if (key->code == sf::Keyboard::Key::Delete && key->control) {
      ctrlDel();
    } else if (key->code == sf::Keyboard::Key::Delete) {
      del();
    } else if (key->code == sf::Keyboard::Key::V && key->control) {
      const auto clip = sf::Clipboard::getString().toAnsiString();
      text_.insert(cursor_pos_, clip);
      cursor_pos_ += clip.size();
    } else if (key->code == sf::Keyboard::Key::Left && key->control) {
      ctrlArrow(false);
    } else if (key->code == sf::Keyboard::Key::Right && key->control) {
      ctrlArrow(true);
    } else if (key->code == sf::Keyboard::Key::Left) {
      if (cursor_pos_ > 0)
        cursor_pos_--;
    } else if (key->code == sf::Keyboard::Key::Right) {
      if (cursor_pos_ < text_.size())
        cursor_pos_++;
    }
    event_bus_.emit(cursor_event_, true);
  } else if (text) {
    if (text->unicode >= 32 && text->unicode < 127) {
      text_.insert(text_.begin() + cursor_pos_, text->unicode);
      cursor_pos_++;
    }
    event_bus_.emit(cursor_event_, true);
  }
}

void Textbox::setCursorSize(const sf::Vector2f &cursor_size) {
  cursor_size_ = cursor_size;
}

void Textbox::setPosition(const sf::Vector2f &pos) {
  pos_ = pos;
}

void Textbox::startEditing() {
  editing_ = true;
  cursor_.start();
}

void Textbox::stopEditing() {
  editing_ = false;
  cursor_.stop();
}

std::size_t Textbox::size() {
  return text_.size();
}

void Textbox::clear() {
  text_.clear();
}

void Textbox::setText(const std::string &text) {
  text_ = text;
}

const std::string &Textbox::getText() const {
  return text_;
}

void Textbox::setCursorPosition(std::size_t pos) {
  cursor_pos_ = pos;
}

std::size_t Textbox::getCursorPosition() const {
  return cursor_pos_;
}

void Textbox::backspace() {
  if (cursor_pos_ > 0) {
    text_.erase(text_.begin() + cursor_pos_ - 1);
    cursor_pos_--;
  }
}

void Textbox::ctrlBackspace() {
  if (cursor_pos_ == 0)
    return;

  std::size_t start = cursor_pos_;
  while (start > 0 && std::isspace(static_cast<unsigned char>(text_[start - 1])))
    --start;

  while (start > 0 && !std::isalnum(static_cast<unsigned char>(text_[start - 1])) &&
         text_[start - 1] != '_' && !std::isspace(static_cast<unsigned char>(text_[start - 1])))
    --start;

  while (start > 0 &&
         (std::isalnum(static_cast<unsigned char>(text_[start - 1])) || text_[start - 1] == '_'))
    --start;

  text_.erase(start, cursor_pos_ - start);
  cursor_pos_ = start;
}
void Textbox::del() {
  if (cursor_pos_ >= text_.size()) {
    backspace();
    return;
  }
  text_.erase(text_.begin() + cursor_pos_);
}

void Textbox::ctrlDel() {
  if (cursor_pos_ >= text_.size()) {
    ctrlBackspace();
    return;
  }
  std::size_t end = cursor_pos_;

  while (end < text_.size() && std::isspace(static_cast<unsigned char>(text_[end])))
    ++end;

  while (end < text_.size() && !std::isalnum(static_cast<unsigned char>(text_[end])) &&
         text_[end] != '_' && !std::isspace(static_cast<unsigned char>(text_[end])))
    ++end;

  while (end < text_.size() &&
         (std::isalnum(static_cast<unsigned char>(text_[end])) || text_[end] == '_'))
    ++end;

  text_.erase(cursor_pos_, end - cursor_pos_);
}

void Textbox::ctrlArrow(bool right) {
  if (right) {
    if (cursor_pos_ >= text_.size())
      return;

    std::size_t pos = cursor_pos_;
    while (pos < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos])))
      ++pos;

    while (pos < text_.size() &&
           (std::isalnum(static_cast<unsigned char>(text_[pos])) || text_[pos] == '_'))
      ++pos;

    while (pos < text_.size() && !std::isalnum(static_cast<unsigned char>(text_[pos])) &&
           text_[pos] != '_' && !std::isspace(static_cast<unsigned char>(text_[pos])))
      ++pos;

    cursor_pos_ = pos;
  } else {
    if (cursor_pos_ == 0)
      return;

    std::size_t pos = cursor_pos_;
    while (pos > 0 && std::isspace(static_cast<unsigned char>(text_[pos - 1])))
      --pos;

    while (pos > 0 && !std::isalnum(static_cast<unsigned char>(text_[pos - 1])) &&
           text_[pos - 1] != '_' && !std::isspace(static_cast<unsigned char>(text_[pos - 1])))
      --pos;

    while (pos > 0 &&
           (std::isalnum(static_cast<unsigned char>(text_[pos - 1])) || text_[pos - 1] == '_'))
      --pos;

    cursor_pos_ = pos;
  }
}

} // namespace ui
