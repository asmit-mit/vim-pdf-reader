#pragma once

#include "core/cmd_processor.h"
#include "core/event_bus.h"
#include "core/history_manager.h"
#include "graphics/font_library.h"
#include "ui/completions.h"
#include "ui/textbox.h"
#include "ui/widget.h"

namespace ui {

enum class CmdlineMode {
  Cmd = 0,
  ForwardSearch,
  BackwardSearch,
};

class Cmdline : public Widget {
public:
  explicit Cmdline(
      const graphics::FontLibrary &font_lib,
      const sf::Font &font_normal,
      const sf::Font &font_bold,
      const sf::Font &font_italic,
      core::EventBus &event_bus,
      core::CmdProcessor &cmd_processor,
      core::HistoryManager &cmd_history,
      core::HistoryManager &search_history
  );

  void draw(sf::RenderTarget &window) const override;
  void update() override;
  void handleEvent(const sf::Event &event) override;

  void setMode(CmdlineMode mode);
  void onResize(const sf::Vector2f &size);

private:
  void reset();
  void refreshCompletions();

private:
  core::EventBus &event_bus_;
  core::CmdProcessor &cmd_processor_;
  core::HistoryManager &cmd_history_;
  core::HistoryManager &search_history_;
  const sf::Font &font_;

  sf::Text label_;
  ui::Textbox textbox_;
  ui::Completions completions_;
  sf::RectangleShape display_area_;

  sf::Vector2f window_size_;
  std::u32string original_string_;

  CmdlineMode mode_;

  bool visible_;
};

} // namespace ui
