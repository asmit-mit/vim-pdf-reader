#pragma once

#include <SFML/Graphics.hpp>

#include "core/event_bus.h"
#include "pdf/pdf_document.h"
#include "pdf/pdf_renderer.h"
#include "ui/scrollwheel.h"

namespace ui {

class PDFView {
public:
  explicit PDFView(
      pdf::PDFDocument &document, pdf::PDFRenderer &renderer, core::EventBus &event_bus
  );

  void draw(sf::RenderTarget &window) const;
  void update();
  void handleEvent(const sf::Event &event);

  void onResize(const sf::Vector2f &size);

private:
  void setZoom(float zoom);
  void setRotate(int rotate);
  void getPage(std::size_t page_num, float zoom, int rotate);
  void centerPage();
  void resetView();
  void setPageLoc(float x, float y);
  float map(float value, float src_min, float src_max, float dst_min, float dst_max);

private:
  core::EventBus &event_bus_;
  pdf::PDFDocument &document_;
  pdf::PDFRenderer &renderer_;
  ui::ScrollWheel horizontal_wheel_;
  ui::ScrollWheel vertical_wheel_;

  sf::Texture texture_;
  sf::Sprite sprite_;
  sf::Clock page_update_timer_;
  sf::Keyboard::Key prev_key_;
  sf::Vector2f window_size_;

  std::size_t current_page_;
  std::size_t render_page_;
  bool has_document_;
  bool should_take_input_;
  bool update_scroll_bar_horizontal_;
  bool update_scroll_bar_vertical_;

  float curr_x_, curr_y_;
  float render_zoom_;
  float visual_zoom_;
  int render_rotate_;
  int visual_rotate_;

  static constexpr float scrollwheel_width_ = 8.f;
  static constexpr float zoom_dobounce_ms_ = 500.f;
  static constexpr float scroll_dist_ = 40.f;
  static constexpr float min_zoom_ = 0.2f;
  static constexpr float max_zoom_ = 5.f;
};

} // namespace ui
