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
  std::size_t findWordStart(std::size_t) const;
  std::size_t findWordEnd(std::size_t) const;

  void backspace();
  void del();
  void ctrlBackspace();
  void ctrlDel();
  void ctrlArrowRight();
  void ctrlArrowLeft();
  void selectAll();

  std::size_t getCharacterIndex(const sf::Vector2i &pos);
  float getCursorX(std::size_t idx);
  void createSelectionBox();
  void rebuildDisplayTexts();

  void deleteSelection();
  void clearSelection();
  std::string getSelectedText() const;

private:
  struct Selection {
    std::size_t anchor = 0;
    std::size_t caret = 0;

    bool active() const {
      return anchor != caret;
    }

    std::size_t begin() const {
      return std::min(anchor, caret);
    }

    std::size_t end() const {
      return std::max(anchor, caret);
    }
  };

private:
  const sf::Font &font_;
  sf::RectangleShape selection_box_;

  sf::Vector2f pos_;

  ui::Cursor cursor_;
  sf::Vector2f cursor_size_;
  bool cursor_dirty_;

  sf::Text display_text_;
  sf::Text display_text_selected_;

  std::string text_;
  bool text_dirty_;

  Selection selection_;
  bool selection_dirty_;
  bool selecting_;

  bool visible_;
  bool editing_;
};

} // namespace ui
