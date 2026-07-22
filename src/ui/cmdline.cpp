#include <stdexcept>

#include "ui/cmdline.h"
#include "ui/ui_elements.h"
#include "utils/settings.h"
#include "utils/utils.h"

namespace ui {

Cmdline::Cmdline(
    const sf::Font &font_normal,
    const sf::Font &font_bold,
    const sf::Font &font_italic,
    core::EventBus &event_bus,
    core::CmdProcessor &cmd_processor,
    core::CmdHistory &cmd_history
)
    : event_bus_(event_bus), cmd_processor_(cmd_processor), cmd_history_(cmd_history),
      font_(font_normal), label_(font_normal, ":", utils::char_size),
      textbox_(font_, utils::char_size, ":"),
      completions_(font_bold, font_italic, utils::char_size) {
  state_ = CmdlineState::Hidden;
  should_take_input_ = false;
  ignore_next_text_entered_ = true;

  curr_x_ = 0.f;
  curr_y_ = 0.f;
  textbox_.setCursorSize({2.f, 24.f});

  label_.setFillColor(utils::hexToRGB(settings::fg_));
  display_area_.setFillColor(utils::hexToRGB(settings::cmd_bg_));
  display_area_.setSize({200.0, height_});

  completions_.setCmdColor(utils::hexToRGB(settings::fg_));
  completions_.setDescColor(utils::hexToRGB(settings::completions_desc_fg_));

  event_bus_.subscribe<ui::UIElements>("ui.focus", [this](ui::UIElements focus) {
    should_take_input_ = focus == ui::UIElements::Cmdline;
    state_ = should_take_input_ ? CmdlineState::Edit : CmdlineState::Hidden;
    textbox_.reset();
    textbox_.startEditing();
  });
}

void Cmdline::draw(sf::RenderTarget &window) const {
  if (state_ == CmdlineState::Hidden)
    return;

  window.draw(display_area_);
  textbox_.draw(window);
  completions_.draw(window);
  window.draw(label_);
}

void Cmdline::update() {
  if (state_ == CmdlineState::Hidden)
    return;

  textbox_.show();
  float round_y = std::round(curr_y_) + 1.f;

  display_area_.setPosition({curr_x_, round_y});
  label_.setPosition({utils::padding - 4.f, round_y});

  if (state_ == CmdlineState::Status) {
    label_.setString("");
    textbox_.setPosition({utils::padding - 1.f, round_y});
  } else {
    label_.setString(":");
    const auto bounds = label_.getGlobalBounds();
    textbox_.setPosition({bounds.position.x + bounds.size.x + 3.f, round_y});
  }

  completions_.update();
  textbox_.update();
}

void Cmdline::handleEvent(const sf::Event &event) {
  if (!should_take_input_)
    return;

  if (ignore_next_text_entered_) {
    if (event.is<sf::Event::TextEntered>()) {
      ignore_next_text_entered_ = false;
      return;
    }
  }

  const auto *key = event.getIf<sf::Event::KeyPressed>();

  if (key) {
    if (state_ == CmdlineState::Status) {
      textbox_.stopEditing();

      if (key->code == sf::Keyboard::Key::Semicolon && key->shift) {
        state_ = CmdlineState::Edit;
        textbox_.reset();
        textbox_.startEditing();
        ignore_next_text_entered_ = true;
      } else if (key->code == sf::Keyboard::Key::Escape) {
        event_bus_.emit("ui.focus", ui::UIElements::PDFView);
        cmd_history_.reset();
        textbox_.reset();
        textbox_.stopEditing();
        textbox_.hide();
        completions_.clear();
        completions_.hide();
      }

      return;
    }

    if (state_ == CmdlineState::Edit) {
      textbox_.startEditing();

      if (key->code == sf::Keyboard::Key::Escape) {
        event_bus_.emit("ui.focus", ui::UIElements::PDFView);
        cmd_history_.reset();
        textbox_.reset();
        textbox_.stopEditing();
        textbox_.hide();
        completions_.clear();
        completions_.hide();
      } else if (key->code == sf::Keyboard::Key::Enter) {
        if (completions_.isVisible()) {
          textbox_.setText(completions_.getSelectedText());
          textbox_.setCursorPosition(textbox_.getText().size());
        } else {
          cmd_history_.add(textbox_.getText());
          try {
            cmd_processor_.runCommand(textbox_.getText());
            textbox_.reset();
            textbox_.stopEditing();
            event_bus_.emit("ui.focus", ui::UIElements::PDFView);
            ignore_next_text_entered_ = true;
          } catch (const std::runtime_error &e) {
            state_ = CmdlineState::Status;
            textbox_.setText(std::string(e.what()) + " (press \":\" to continue...)");
            textbox_.setCursorPosition(textbox_.getText().size());
            textbox_.stopEditing();
          }
        }
        completions_.clear();
        completions_.hide();
      } else if (
          (key->code == sf::Keyboard::Key::Up) ||
          (key->code == sf::Keyboard::Key::P && key->control)
      ) {
        textbox_.setText(cmd_history_.getPrevious());
        textbox_.setCursorPosition(textbox_.getText().size());
      } else if (
          key->code == sf::Keyboard::Key::Down ||
          (key->code == sf::Keyboard::Key::N && key->control)
      ) {
        textbox_.setText(cmd_history_.getNext());
        textbox_.setCursorPosition(textbox_.getText().size());
      } else if (key->code == sf::Keyboard::Key::Tab) {
        if (!completions_.isVisible()) {
          const auto &list = cmd_processor_.complete(textbox_.getText());
          if (list.empty()) {
            completions_.clear();
            completions_.hide();
          } else if (list.size() == 1) {
            textbox_.setText(list[0].first);
            textbox_.setCursorPosition(list[0].first.size());
          } else {
            completions_.setCompletionList(list);
            completions_.show();
          }
        } else {
          if (key->shift) {
            completions_.moveUp();
            return;
          }

          if (string_at_last_tab_ == textbox_.getText()) {
            completions_.moveDown();
          } else {
            const auto &list = cmd_processor_.complete(textbox_.getText());
            if (list.empty()) {
              completions_.clear();
              completions_.hide();
            } else if (list.size() == 1) {
              textbox_.setText(list[0].first);
              textbox_.setCursorPosition(list[0].first.size());
              completions_.clear();
              completions_.hide();
            } else {
              completions_.setCompletionList(list);
              completions_.show();
            }
            string_at_last_tab_ = textbox_.getText();
          }
        }
      }
    }
  }

  if (state_ == CmdlineState::Edit)
    textbox_.handleEvent(event);
}

void Cmdline::onResize(const sf::Vector2f &size) {
  curr_x_ = 0.f;
  curr_y_ = size.y - height_;
  display_area_.setSize({size.x, height_});
  completions_.onResize(size);
}

} // namespace ui
