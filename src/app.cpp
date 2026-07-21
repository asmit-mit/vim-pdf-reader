#include "app.h"
#include "utils/settings.h"
#include "utils/utils.h"

App::App()
    : font_normal_(settings::font_normal_), font_bold_(settings::font_bold_),
      font_italic_(settings::font_italic_), cmd_processor_(event_bus_), renderer_(document_),
      cmdline_(font_normal_, font_bold_, font_italic_, event_bus_, cmd_processor_, cmd_history_),
      statusbar_(font_normal_, event_bus_), pdf_view_(document_, renderer_, event_bus_) {
  sf::ContextSettings settings;
  settings.antiAliasingLevel = 8;

  window_ = sf::
      RenderWindow(sf::VideoMode({res_x_, res_y_}), "Vim PDF Reader", (sf::Style::Resize + sf::Style::Close), sf::State::Windowed, settings);
  focus_ = ui::UIElements::PDFView;

  window_.setFramerateLimit(fps_);
  window_.setKeyRepeatEnabled(true);

  view_ = window_.getDefaultView();

  cmdline_.onResize(view_.getSize());
  statusbar_.onResize(view_.getSize());
  pdf_view_.onResize(view_.getSize());

  event_bus_.subscribe<bool>("cmd_processor.quit", [this](bool close) { window_.close(); });

  event_bus_.subscribe<ui::UIElements>("ui.focus", [this](ui::UIElements focus) {
    focus_ = focus;
  });
}

void App::run() {
  while (window_.isOpen()) {
    processEvents();

    pdf_view_.update();
    cmdline_.update();
    statusbar_.update();

    window_.clear(utils::hexToRGB(settings::bg_));
    window_.setView(view_);

    pdf_view_.draw(window_);
    cmdline_.draw(window_);
    statusbar_.draw(window_);

    window_.display();
  }
}

void App::processEvents() {
  while (const auto event = window_.pollEvent()) {
    if (event->is<sf::Event::Closed>())
      window_.close();

    if (event->is<sf::Event::Resized>()) {
      auto window_size = window_.getSize();
      view_.setSize({(float)window_size.x, (float)window_size.y});
      auto view_size = view_.getSize();
      view_.setCenter({view_size.x / 2, view_size.y / 2});

      cmdline_.onResize(view_.getSize());
      statusbar_.onResize(view_.getSize());
      pdf_view_.onResize(view_.getSize());
    }

    if (focus_ == ui::UIElements::PDFView) {
      if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Semicolon && key->shift &&
            focus_ != ui::UIElements::Cmdline)
          event_bus_.emit("ui.focus", ui::UIElements::Cmdline);
      }
    }

    cmdline_.handleEvent(*event);
    statusbar_.handleEvent(*event);
    pdf_view_.handleEvent(*event);
  }
}
