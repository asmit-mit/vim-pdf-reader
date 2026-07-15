#include <stdexcept>

#include "ui/cmdline.h"
#include "ui/ui_elements.h"
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
      label_(font, ":", 16),
      textbox_(font_, event_bus, 16, ":", "cmdline.visible", "cmdline.typing") {
  state_ = CmdlineState::Hidden;
  should_take_input_ = false;
  ignore_next_text_entered_ = false;

  curr_x_ = 0.f;
  curr_y_ = 0.f;
  textbox_.setCursorSize({2.f, 24.f});

  label_.setFillColor(utils::hexToRGB(settings::fg_));
  display_area_.setFillColor(utils::hexToRGB(settings::cmd_bg_));
  display_area_.setSize({200.0, height_});

  event_bus_.subscribe<ui::UIElements>("ui.focus", [this](ui::UIElements focus) {
    should_take_input_ = focus == ui::UIElements::Cmdline;
    state_ = should_take_input_ ? CmdlineState::Edit : CmdlineState::Hidden;
    textbox_.clear();
    textbox_.setCursorPosition(0);
    textbox_.startEditing();
  });
}

void Cmdline::draw(sf::RenderTarget &window) const {
  if (state_ == CmdlineState::Hidden)
    return;

  window.draw(display_area_);
  textbox_.draw(window);
  window.draw(label_);
}

void Cmdline::update() {
  if (state_ == CmdlineState::Hidden) {
    event_bus_.emit("cmdline.visible", false);
    return;
  }

  event_bus_.emit("cmdline.visible", true);

  display_area_.setPosition({curr_x_, curr_y_});
  label_.setPosition({utils::padding - 4.f, curr_y_});

  if (state_ == CmdlineState::Status) {
    label_.setString("");
    textbox_.setPosition({utils::padding - 1.f, curr_y_});
  } else {
    label_.setString(":");
    const auto bounds = label_.getGlobalBounds();
    textbox_.setPosition({bounds.position.x + bounds.size.x + 3.f, curr_y_});
  }

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
      label_.setString("");
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
      }

      return;
    }

    if (state_ == CmdlineState::Edit) {
      label_.setString(":");
      textbox_.startEditing();

      if (key->code == sf::Keyboard::Key::Escape) {
        event_bus_.emit("ui.focus", ui::UIElements::PDFView);
        cmd_history_.reset();
        textbox_.reset();
        textbox_.stopEditing();
      } else if (key->code == sf::Keyboard::Key::Enter) {
        cmd_history_.add(textbox_.getText());
        try {
          cmd_processor_.runCommand(textbox_.getText());
          textbox_.reset();
          textbox_.stopEditing();
          event_bus_.emit("ui.focus", ui::UIElements::PDFView);
        } catch (const std::runtime_error &e) {
          state_ = CmdlineState::Status;
          textbox_.setText(std::string(e.what()) + " (press \":\" to continue...)");
          textbox_.setCursorPosition(textbox_.getText().size());
          textbox_.stopEditing();
        }
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
}

} // namespace ui
