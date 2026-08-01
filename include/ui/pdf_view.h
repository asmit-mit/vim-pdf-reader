#pragma once

#include <SFML/Graphics.hpp>

#include "core/event_bus.h"
#include "core/render_scheduler.h"
#include "pdf/pdf_document.h"
#include "ui/page_view.h"

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

  void syncWithTargetState();
  void setInitialPagePos();
  void renderRequestedPages();
  void updatePagePositions();
  void checkForAnchorPage();

  void setZoom(float zoom);
  void setRotate(int rotate);
  void requestPage(std::size_t page_idx, float zoom, int rotate);
  void resetView();
  void panCurrentPage(sf::Vector2f delta);

  void putPageInNonFracPos(PageView &page);
  void updateNeighbourPositions();
  float map(float value, float src_min, float src_max, float dst_min, float dst_max);

private:
  struct ScrollWheelDirty {
    bool horizontal = false;
    bool vertical = false;
  };

  struct VisualInfo {
    float zoom = 1.f;
    int rotate = 0;
  };

private:
  std::vector<PageView> pages_;

  core::EventBus &event_bus_;
  pdf::PDFDocument &document_;
  core::RenderScheduler &scheduler_;

  // ui::ScrollWheel horizontal_wheel_;
  // ui::ScrollWheel vertical_wheel_;
  // ScrollWheelDirty scrollwheel_dirty_;

  sf::Texture dummy_;
  sf::Clock scale_rot_update_timer_;
  VisualInfo target_state_;

  std::size_t anchor_page_;
  std::size_t front_page_;
  std::size_t back_page_;

  bool has_document_;
  bool should_take_input_;
  bool need_initial_pos_;
  bool window_size_changed_;
  bool pending_page_update_;
  bool started_scrolling_;

  sf::Vector2f window_size_;
  sf::Vector2f old_window_size_;
  sf::Keyboard::Key prev_key_ = sf::Keyboard::Key::Unknown;

  std::string filepath_;

  static constexpr float gap_ = 1.f;
  static constexpr float scrollwheel_width_ = 8.f;
  static constexpr float scale_rot_debounce_ms_ = 300.f;
  static constexpr float scroll_dist_ = 40.f;
  static constexpr float min_zoom_ = 0.2f;
  static constexpr float max_zoom_ = 5.f;
};

} // namespace ui
