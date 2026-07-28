#pragma once

#include <SFML/Graphics.hpp>

#include "core/event_bus.h"
#include "core/render_scheduler.h"
#include "pdf/pdf_document.h"
#include "pdf/pdf_renderer.h"
#include "ui/scrollwheel.h"

namespace ui {

class PDFView {
public:
  explicit PDFView(
      pdf::PDFDocument &document, core::RenderScheduler &scheduler, core::EventBus &event_bus
  );

  void draw(sf::RenderTarget &window) const;
  void update();
  void handleEvent(const sf::Event &event);

  void onResize(const sf::Vector2f &size);

private:
  void onOpenDocument(const std::string &filepath);
  void onCloseDocument();
  void onSwitchPage(int page_num);

  void requestRenderIfChanged();
  void applyReadyRender();
  void updateSpriteTransform();
  void refreshScrollbars();

  void setZoom(float zoom);
  void setRotate(int rotate);
  void requestPage(std::size_t page_num, float zoom, int rotate);
  void syncScaleRotation();
  void centerPage();
  void resetView();
  void setPageLoc(float x, float y);
  static float map(float value, float src_min, float src_max, float dst_min, float dst_max);

private:
  struct ViewTransform {
    float zoom = 1.f;
    int rotate = 0;
  };

  struct PendingRender {
    pdf::PDFRenderKey key;
    bool active = false;
  };

  struct PageSizeCache {
    bool valid = false;
    std::size_t page = 0;
    float zoom = 0.f;
    int rotate = 0;
    sf::Vector2u size;
  };

  struct ScrollbarDirty {
    bool horizontal = false;
    bool vertical = false;
  };

private:
  core::EventBus &event_bus_;
  pdf::PDFDocument &document_;
  core::RenderScheduler &scheduler_;

  ui::ScrollWheel horizontal_wheel_;
  ui::ScrollWheel vertical_wheel_;

  sf::Texture texture_;
  sf::Sprite sprite_;
  sf::Clock page_update_timer_;
  sf::Keyboard::Key prev_key_ = sf::Keyboard::Key::Unknown;

  sf::Vector2f window_size_{0.f, 0.f};
  sf::Vector2f page_loc_{0.f, 0.f};

  bool has_document_ = false;
  bool should_take_input_ = false;
  bool needs_initial_center_ = false;

  std::size_t current_page_ = 0;
  std::size_t render_page_ = 0;

  ViewTransform visual_;
  ViewTransform render_;
  PendingRender pending_;

  PageSizeCache size_cache_;
  ScrollbarDirty scrollbar_dirty_;

  static constexpr float scrollwheel_width_ = 8.f;
  static constexpr float zoom_debounce_ms_ = 300.f;
  static constexpr float scroll_dist_ = 40.f;
  static constexpr float min_zoom_ = 0.2f;
  static constexpr float max_zoom_ = 5.f;
};

} // namespace ui
