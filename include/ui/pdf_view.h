#pragma once

#include <SFML/Graphics.hpp>

#include "core/event_bus.h"
#include "core/history_manager.h"
#include "core/render_scheduler.h"
#include "pdf/pdf_document.h"
#include "pdf/pdf_layout_manager.h"
#include "pdf/pdf_view_controller.h"
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

  void renderRequestedPages();

  void requestPage(std::size_t page_idx, float zoom, int rotate);
  void resetView();
  void goNextPageWithResult();
  void goPrevPageWithResult();

private:
  pdf::PDFDocument &document_;
  pdf::PDFLayoutManager layout_manager_;
  pdf::PDFViewController view_controller_;

  std::vector<PageView> pages_;

  core::EventBus &event_bus_;
  core::HistoryManager &file_history_;
  core::RenderScheduler &scheduler_;

  sf::Texture dummy_;

  std::size_t curr_search_result_;
  std::size_t local_search_result_;
  std::size_t total_search_results_;
  std::vector<std::size_t> pages_with_search_results_;

  bool has_document_;
  bool should_take_input_;
  bool window_size_changed_;
  bool search_pos_dirty_;
  bool show_search_result_boxes_;

  sf::Vector2f old_window_size_;
  sf::Vector2f window_size_;

  std::string filepath_;

  static constexpr float scrollwheel_width_ = 8.f;
  static constexpr float scroll_dist_ = 40.f;
};

} // namespace ui
