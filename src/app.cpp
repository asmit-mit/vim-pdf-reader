#include "app.h"
#include "utils/settings.h"
#include "utils/utils.h"

App::App()
    : font_(
          "/home/asmitpaul/.local/share/fonts/"
          "JetBrainsMonoNerdFont-Light.ttf"
      ),
      cmdline(font_) {
  window_ = sf::
      RenderWindow(sf::VideoMode({res_x_, res_y_}), "Vim PDF Reader", (sf::Style::Resize + sf::Style::Close));
  window_.setFramerateLimit(fps_);
  window_.setKeyRepeatEnabled(true);

  view_ = window_.getDefaultView();

  cmdline.onResize(view_.getSize());
}

void App::run() {
  while (window_.isOpen()) {
    processEvents();

    cmdline.update();

    window_.clear(utils::hexToRGB(settings::bg_));
    window_.setView(view_);
    cmdline.draw(window_);
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

      cmdline.onResize(view_.getSize());
    }

    cmdline.handleEvent(*event);
  }
}
