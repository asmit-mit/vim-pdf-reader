#include <print>
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
    core::HistorySaver &cmd_history,
    core::HistorySaver &search_history
)
    : event_bus_(event_bus), cmd_processor_(cmd_processor), cmd_history_(cmd_history),
      search_history_(search_history), font_(font_normal),
      label_(font_normal, ":", utils::char_size), textbox_(font_, utils::char_size, ":"),
      completions_(font_bold, font_italic, utils::char_size) {
  visible_ = false;
  ignore_next_text_entered_ = false;

  textbox_.setCursorSize({2.f, 24.f});

  label_.setFillColor(utils::hexToRGB(settings::fg_));

  display_area_.setFillColor(utils::hexToRGB(settings::cmd_bg_));
  display_area_.setSize({200.0, utils::cmdline_height_});

  completions_.setCmdColor(utils::hexToRGB(settings::fg_));
  completions_.setDescColor(utils::hexToRGB(settings::completions_desc_fg_));

  event_bus_.subscribe<ui::UIElements>("ui.focus", [this](ui::UIElements focus) {
    visible_ = focus == ui::UIElements::Cmdline;
    ignore_next_text_entered_ = visible_;
    if (visible_) {
      textbox_.reset();
      textbox_.startEditing();
      textbox_.show();
    }
  });
}

void Cmdline::draw(sf::RenderTarget &window) const {
  if (!visible_)
    return;

  window.draw(display_area_);
  textbox_.draw(window);
  completions_.draw(window);
  window.draw(label_);
}

void Cmdline::update() {
  if (!visible_)
    return;

  completions_.update();
  textbox_.update();
}

void Cmdline::handleEvent(const sf::Event &event) {
  if (!visible_)
    return;

  if (const auto *te = event.getIf<sf::Event::TextEntered>()) {
    if (te->unicode == '\t')
      return;
  }

  if (ignore_next_text_entered_) {
    if (event.is<sf::Event::TextEntered>()) {
      ignore_next_text_entered_ = false;
      return;
    }
  }

  const auto *key = event.getIf<sf::Event::KeyPressed>();

  if (mode_ == CmdlineMode::Cmd) {
    if (completions_.isVisible()) {
      bool is_edit = event.is<sf::Event::TextEntered>();
      if (!is_edit && key)
        is_edit = key->code == sf::Keyboard::Key::Backspace ||
                  key->code == sf::Keyboard::Key::Delete;
      if (is_edit) {
        completions_.clear();
        completions_.hide();
      }
    }

    if (key) {
      if (key->code == sf::Keyboard::Key::Escape) {
        if (completions_.isVisible()) {
          completions_.clear();
          completions_.hide();
          textbox_.setText(original_string_);
          return;
        }
        event_bus_.emit("ui.focus", ui::UIElements::PDFView);
        cmd_history_.reset();
      } else if (key->code == sf::Keyboard::Key::Enter) {
        cmd_history_.add(textbox_.getText());
        try {
          cmd_processor_.runCommand(textbox_.getText());
        } catch (const std::runtime_error &e) {
          event_bus_.emit("cmdline.msg", e.what());
        }
        event_bus_.emit("ui.focus", ui::UIElements::ErrorLine);
        reset();
      } else if (key->code == sf::Keyboard::Key::P && key->control) {
        textbox_.setText(cmd_history_.getPrevious());
      } else if (key->code == sf::Keyboard::Key::N && key->control) {
        textbox_.setText(cmd_history_.getNext());
      } else if (key->code == sf::Keyboard::Key::Tab) {
        if (!completions_.isVisible()) {
          original_string_ = textbox_.getText();
          refreshCompletions();
        } else {
          if (key->shift)
            completions_.moveUp();
          else
            completions_.moveDown();
          textbox_.setText(completions_.getSelectedText());
        }
        return;
      }
    }
  } else {
    if (key) {
      if (key->code == sf::Keyboard::Key::Escape) {
        event_bus_.emit("ui.focus", ui::UIElements::PDFView);
      } else if (key->code == sf::Keyboard::Key::Enter) {
        std::println("Text to search for: {}", textbox_.getText());
        search_history_.add(textbox_.getText());
        event_bus_.emit("ui.focus", ui::UIElements::PDFView);
      } else if (key->code == sf::Keyboard::Key::P && key->control) {
        textbox_.setText(search_history_.getPrevious());
      } else if (key->code == sf::Keyboard::Key::N && key->control) {
        textbox_.setText(search_history_.getNext());
      } else if (key->code == sf::Keyboard::Key::N) {
        if (key->shift) {
          if (mode_ == CmdlineMode::ForwardSearch) {
            // search backwrad
          } else {
            // search forward
          }
        } else {
          if (mode_ == CmdlineMode::ForwardSearch) {
            // search forward
          } else {
            // search backward
          }
        }
      }
    }
  }

  textbox_.handleEvent(event);
}

void Cmdline::setMode(CmdlineMode mode) {
  mode_ = mode;
  if (mode_ == CmdlineMode::Cmd)
    label_.setString(":");
  else if (mode_ == CmdlineMode::ForwardSearch)
    label_.setString("/");
  else if (mode_ == CmdlineMode::BackwardSearch)
    label_.setString("?");
}

void Cmdline::onResize(const sf::Vector2f &size) {
  window_size_ = size;
  display_area_.setSize({size.x, utils::cmdline_height_});
  completions_.onResize(size);

  const float curr_x_ = 0.f;
  const float curr_y_ = window_size_.y - utils::cmdline_height_;
  float round_y = std::round(curr_y_) + 1.f;

  display_area_.setPosition({curr_x_, round_y});
  label_.setPosition({utils::padding - 4.f, round_y});

  const auto bounds = label_.getGlobalBounds();
  textbox_.setPosition({bounds.position.x + bounds.size.x + 3.f, round_y});
}

void Cmdline::reset() {
  textbox_.reset();
  textbox_.stopEditing();
  textbox_.hide();
  completions_.clear();
  completions_.hide();
}

void Cmdline::refreshCompletions() {
  const auto &list = cmd_processor_.complete(original_string_);

  if (list.empty()) {
    completions_.clear();
    completions_.hide();
    return;
  }

  if (list.size() == 1) {
    textbox_.setText(list[0].first);
    completions_.clear();
    completions_.hide();
    return;
  }

  completions_.setCompletionList(list);
  completions_.show();
  textbox_.setText(completions_.getSelectedText());
}

} // namespace ui
