#pragma once

#include "core/event_bus.h"
#include "ui/cursor.h"
#include "ui/widget.h"

namespace ui {

class Textbox : public Widget {
public:
  explicit Textbox(
      const sf::Font &font,
      core::EventBus &event_bus,
      int char_size,
      const std::string &input_string,
      const std::string &enable_event,
      const std::string &typing_event
  );

  void draw(sf::RenderTarget &window) const override;
  void update() override;
  void handleEvent(const sf::Event &event) override;

  void setCursorSize(const sf::Vector2f &cursor_size);
  void setPosition(const sf::Vector2f &pos);

  void startEditing();
  void stopEditing();

  std::size_t size();
  void clear();

  void setText(const std::string &text);
  const std::string &getText() const;

  void setCursorPosition(std::size_t pos);
  std::size_t getCursorPosition() const;

  void backspace();
  void ctrlBackspace();
  void del();
  void ctrlDel();
  void ctrlArrow(bool direction);

private:
  core::EventBus &event_bus_;
  const sf::Font &font_;
  sf::Vector2f cursor_size_;
  sf::Vector2f pos_;
  std::size_t cursor_pos_;

  sf::Text display_text_;
  ui::Cursor cursor_;

  std::string text_;
  const std::string cursor_event_;

  bool visible_;
  bool editing_;
};

} // namespace ui
