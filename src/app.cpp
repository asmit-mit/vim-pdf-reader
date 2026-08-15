#include "app.h"
#include "core/render_scheduler.h"
#include "graphics/font_library.h"
#include "ui/ui_elements.h"
#include "utils/settings.h"
#include "utils/utils.h"

#include <filesystem>

App::App()
    : font_regular_(settings::font_regular), font_bold_(settings::font_bold),
      font_italic_(settings::font_italic), document_(), renderer_(),
      cmd_loader_("./config/commands.json"), cmd_processor_(event_bus_, cmd_loader_, cmd_history_),
      render_scheduler_(document_, renderer_, settings::thread_count_),
      cmdline_textbox_(font_library_, glyph_atlas_, utils::char_size, ":"),
      cmdline_completions_(font_library_, glyph_atlas_, font_regular_, font_italic_, utils::char_size),
      cmdline_(font_regular_, event_bus_, cmd_processor_, cmd_history_, search_history_, cmdline_textbox_, cmdline_completions_),
      pdf_view_(file_history_, document_, render_scheduler_, event_bus_),
      statusbar_(font_regular_, event_bus_),
      notifications_(font_library_, glyph_atlas_, font_regular_, font_bold_, notification_history_, event_bus_) {
  initHistory();
  initWindow();
  initUI();
  initFonts();

  event_bus_.subscribe<bool>("cmd.quit", [this](bool close) {
    renderer_.clearCache();
    document_.closeDocument();
    window_.close();
  });

  event_bus_.subscribe<ui::UIElements>("ui.focus", [this](ui::UIElements focus) {
    focus_ = focus;
  });
}

void App::run() {
  while (window_.isOpen()) {
    processEvents();

    event_bus_.update();
    pdf_view_.update();
    cmdline_.update();
    statusbar_.update();
    notifications_.update();

    window_.clear(utils::hexToRGB(settings::bg_));
    window_.setView(view_);

    pdf_view_.draw(window_);
    cmdline_.draw(window_);
    statusbar_.draw(window_);
    notifications_.draw(window_);

    window_.display();
  }
}

void App::initHistory() {
  const char *home = std::getenv("HOME");
  if (!home)
    home = ".";

  std::string state_dir_ = std::string(home) + "/.local/state/vim-pdf-reader";
  std::filesystem::create_directory(state_dir_);

  cmd_history_.setPath(state_dir_ + "/cmd_history");
  file_history_.setPath(state_dir_ + "/recent_files");
  search_history_.setPath(state_dir_ + "/search_history");

  file_history_.setSaveUnique(true);

  event_bus_.subscribe<bool>("cmd.clear_files", [this](bool) { file_history_.clear(); });
  event_bus_.subscribe<bool>("cmd.clear_search", [this](bool) { search_history_.clear(); });
  event_bus_.subscribe<bool>("cmd.clear_history", [this](bool) { cmd_history_.clear(); });
}

void App::initWindow() {
  sf::ContextSettings settings;
  settings.antiAliasingLevel = 8;

  window_ = sf::
      RenderWindow(sf::VideoMode({res_x_, res_y_}), "Vim PDF Reader", (sf::Style::Resize + sf::Style::Close), sf::State::Windowed, settings);
  focus_ = ui::UIElements::PDFView;

  window_.setFramerateLimit(fps_);
  window_.setKeyRepeatEnabled(true);

  view_ = window_.getDefaultView();
}

void App::initUI() {
  cmdline_.onResize(view_.getSize());
  statusbar_.onResize(view_.getSize());
  pdf_view_.onResize(view_.getSize());
}

void App::initFonts() {
  font_library_.tryLoadFont(graphics::FontType::Regular, settings::font_regular);
  font_library_.tryLoadFont(graphics::FontType::Bold, settings::font_bold);
  font_library_.tryLoadFont(graphics::FontType::Italic, settings::font_italic);
  font_library_.tryLoadFont(graphics::FontType::Emoji, settings::font_emoji);
  font_library_.tryLoadFont(graphics::FontType::CJK, settings::font_cjk);

  font_regular_.setSmooth(false);
  font_bold_.setSmooth(false);
  font_italic_.setSmooth(false);
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
      notifications_.onResize(view_.getSize());
    }

    if (focus_ == ui::UIElements::PDFView) {
      if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Semicolon && key->shift &&
            focus_ != ui::UIElements::Cmdline) {
          event_bus_.emit("ui.focus", ui::UIElements::Cmdline);
          cmdline_.setMode(ui::CmdlineMode::Cmd);
        }
        if (key->code == sf::Keyboard::Key::Slash && focus_ != ui::UIElements::Cmdline) {
          event_bus_.emit("ui.focus", ui::UIElements::Cmdline);
          if (key->shift)
            cmdline_.setMode(ui::CmdlineMode::BackwardSearch);
          else
            cmdline_.setMode(ui::CmdlineMode::ForwardSearch);
        }
      }
    }

    cmdline_.handleEvent(*event);
    statusbar_.handleEvent(*event);
    pdf_view_.handleEvent(*event);
    notifications_.handleEvent(*event);
  }
}
