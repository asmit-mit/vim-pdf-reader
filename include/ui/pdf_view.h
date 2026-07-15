#pragma once

#include <SFML/Graphics.hpp>

#include "core/event_bus.h"
#include "pdf/pdf_document.h"
#include "pdf/pdf_renderer.h"

namespace ui {

class PDFView {
public:
  explicit PDFView(core::EventBus &event_bus);

  void draw(sf::RenderTarget &window) const;
  void update();
  void handleEvent(const sf::Event &event);

  void onResize(const sf::Vector2f &size);
  void setZoom(float zoom);

private:
  pdf::PDFDocument document_;
  pdf::PDFRenderer renderer_;
  core::EventBus &event_bus_;

  sf::Texture texture_;
  sf::Sprite sprite_;

  bool has_document_ = false;
  std::size_t current_page_ = 0;
  bool zoom_changed_ = false;

  sf::Vector2f page_size_;
  float curr_x_, curr_y_;
  float zoom_ = 1.f;

  static constexpr float min_zoom_ = 0.2f;
  static constexpr float max_zoom_ = 5.f;
};

} // namespace ui
