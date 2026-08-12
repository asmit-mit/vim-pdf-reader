#pragma once

#include <SFML/Graphics.hpp>
#include <mupdf/fitz.h>

#include <mutex>
#include <string>
#include <vector>

namespace pdf {

struct PDFPage {
  std::size_t page_idx{};
  fz_rect page_bounds{};

  std::u32string text;
  std::u32string text_normalized;
  std::vector<fz_rect> glyphs;

  bool content_loaded{false};

  PDFPage() = default;
  PDFPage(std::size_t idx, fz_rect bounds);

  PDFPage(const PDFPage &) = delete;
  PDFPage &operator=(const PDFPage &) = delete;
  PDFPage(PDFPage &&) noexcept;
  PDFPage &operator=(PDFPage &&) noexcept;

  void loadContent(fz_stext_page *page_text);
  bool isContentLoaded() const;

  std::vector<std::vector<fz_rect>> searchText(const std::u32string &pattern) const;

private:
  mutable std::mutex mutex_;
};

} // namespace pdf
