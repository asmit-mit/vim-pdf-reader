#pragma once

#include <SFML/Graphics.hpp>

#include "core/event_bus.h"
#include "pdf/pdf_document.h"
#include "pdf/pdf_renderer.h"

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

private:
  core::EventBus &event_bus_;
  pdf::PDFDocument &document_;
  pdf::PDFRenderer &renderer_;

  sf::Texture texture_;
  sf::Sprite sprite_;
  sf::Clock zoom_timer_;
  sf::Vector2f window_size_;

  std::size_t current_page_;
  bool has_document_;
  bool zoom_changed_;
  bool page_changed_;
  bool should_take_input_;

  float curr_x_, curr_y_;
  float render_zoom_;
  float visual_zoom_;

  static constexpr float zoom_dobounce_ms_ = 500.f;
  static constexpr float scroll_dist_ = 40.f;
  static constexpr float min_zoom_ = 0.2f;
  static constexpr float max_zoom_ = 5.f;
};

} // namespace ui
