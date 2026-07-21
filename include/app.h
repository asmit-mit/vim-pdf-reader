#pragma once

#include "SFML/Graphics.hpp"

#include "core/cmd_history.h"
#include "core/cmd_processor.h"
#include "core/event_bus.h"
#include "ui/cmdline.h"
#include "ui/pdf_view.h"
#include "ui/statusbar.h"
#include "ui/ui_elements.h"

class App {
public:
  App();

  void run();

private:
  void processEvents();

private:
  sf::RenderWindow window_;
  sf::View view_;
  sf::Font font_normal_;
  sf::Font font_bold_;
  sf::Font font_italic_;

  core::EventBus event_bus_;
  core::CmdProcessor cmd_processor_;
  core::CmdHistory cmd_history_;

  pdf::PDFDocument document_;
  pdf::PDFRenderer renderer_;

  ui::Cmdline cmdline_;
  ui::Statusbar statusbar_;
  ui::PDFView pdf_view_;
  ui::UIElements focus_;

  const unsigned int res_x_ = 640;
  const unsigned int res_y_ = 480;
  const unsigned int fps_ = 60;
};
