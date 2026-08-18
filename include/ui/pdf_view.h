#pragma once

#include <SFML/Graphics.hpp>

#include "core/event_bus.h"
#include "core/history_manager.h"
#include "core/render_scheduler.h"
#include "pdf/pdf_document.h"
#include "ui/page_view.h"

namespace ui {

class PDFView {
public:
  explicit PDFView(
      core::HistoryManager &file_history,
      pdf::PDFDocument &document,
      core::RenderScheduler &scheduler,
      core::EventBus &event_bus
  );

  void draw(sf::RenderTarget &window) const;
  void update();
  void handleEvent(const sf::Event &event);

  void onResize(const sf::Vector2f &size);

private:
  void onOpenDocument(const std::string &filepath);
  void onCloseDocument();
  void onSwitchPage(int page_idx);
  void onSearchPage(const std::u32string &text);

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
  void goNextPageWithResult();
  void goPrevPageWithResult();

  void discretizePosition(PageView &page);
  void updateNeighbourPositions();
  void clampAnchorHorizontally();
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
  core::HistoryManager &file_history_;
  pdf::PDFDocument &document_;
  core::RenderScheduler &scheduler_;

  sf::Texture dummy_;
  sf::Clock scale_rot_update_timer_;
  VisualInfo target_state_;

  std::size_t anchor_page_;
  std::size_t front_page_;
  std::size_t back_page_;

  std::size_t curr_search_result_;
  std::size_t local_search_result_;
  std::size_t total_search_results_;
  std::vector<std::size_t> pages_with_search_results_;

  bool has_document_;
  bool should_take_input_;
  bool need_initial_pos_;
  bool window_size_changed_;
  bool pending_page_update_;
  bool started_scrolling_;
  bool page_positions_dirty_;
  bool sync_state_dirty_;
  bool search_pos_dirty_;
  bool show_search_result_boxes_;

  sf::Vector2f window_size_;
  sf::Vector2f old_window_size_;
  sf::Vector2f anchor_page_pos_before_scroll_;

  std::string filepath_;
  std::size_t page_with_max_width_;

  static constexpr float gap_ = 1.f;
  static constexpr float scrollwheel_width_ = 8.f;
  static constexpr float scale_rot_debounce_ms_ = 300.f;
  static constexpr float scroll_dist_ = 40.f;
  static constexpr float min_zoom_ = 0.2f;
  static constexpr float max_zoom_ = 5.f;
  static constexpr float epsilon_ = 1e-5f;
};

} // namespace ui
