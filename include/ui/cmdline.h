#pragma once

#include "core/cmd_history.h"
#include "core/cmd_processor.h"
#include "core/event_bus.h"
#include "ui/textbox.h"
#include "ui/widget.h"

namespace ui {

enum class CmdlineState { Status, Edit, Hidden };

class Cmdline : public Widget {
public:
  explicit Cmdline(
      const sf::Font &font,
      core::EventBus &event_bus,
      core::CmdProcessor &cmd_processor,
      core::CmdHistory &cmd_history
  );

  void draw(sf::RenderTarget &window) const override;
  void update() override;
  void handleEvent(const sf::Event &event) override;

  void onResize(const sf::Vector2f &size);

private:
  void handleSpecialBackspace();

private:
  core::EventBus &event_bus_;
  core::CmdProcessor &cmd_processor_;
  core::CmdHistory &cmd_history_;
  const sf::Font &font_;

  sf::Text label_;
  ui::Textbox textbox_;
  sf::RectangleShape display_area_;
  CmdlineState state_;

  float curr_x_, curr_y_;
  static constexpr float height_ = 24.0;
};

} // namespace ui
