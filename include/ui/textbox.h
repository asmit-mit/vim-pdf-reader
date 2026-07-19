#pragma once

#include "ui/cursor.h"
#include "ui/widget.h"

namespace ui {

class Textbox : public Widget {
public:
  explicit Textbox(const sf::Font &font, int char_size, const std::string &input_string);

  void draw(sf::RenderTarget &window) const override;
  void update() override;
  void handleEvent(const sf::Event &event) override;

  void setCursorSize(const sf::Vector2f &cursor_size);
  void setPosition(const sf::Vector2f &pos);

  void show();
  void hide();
  void startEditing();
  void stopEditing();

  std::size_t size();
  void clear();
  void reset();

  void setText(const std::string &text);
  const std::string &getText() const;

  void setCursorPosition(int pos);
  std::size_t getCursorPosition() const;

private:
  void backspace();
  void ctrlBackspace();
  void del();
  void ctrlDel();
  void ctrlArrow(bool direction);

private:
  const sf::Font &font_;
  sf::Vector2f cursor_size_;
  sf::Vector2f pos_;
  std::size_t cursor_pos_;

  sf::Text display_text_;
  ui::Cursor cursor_;

  std::string text_;

  bool visible_;
  bool editing_;
  bool text_dirty_;
  bool cursor_dirty_;
};

} // namespace ui
