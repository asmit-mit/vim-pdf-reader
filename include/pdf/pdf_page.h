// PDFPage.hpp

#pragma once

#include <mupdf/fitz.h>

#include <string>
#include <vector>

namespace pdf {

struct Glyph {
  char32_t codepoint;
  fz_quad quad;
};

struct PDFPage {
  std::size_t page_idx{};
  fz_rect page_bounds{};

  std::u32string text;
  std::vector<Glyph> glyphs;

  PDFPage() = default;

  PDFPage(fz_context *ctx, std::size_t idx, fz_rect bounds, fz_stext_page *page_text);

  PDFPage(const PDFPage &) = delete;
  PDFPage &operator=(const PDFPage &) = delete;

  PDFPage(PDFPage &&) noexcept = default;
  PDFPage &operator=(PDFPage &&) noexcept = default;
};

} // namespace pdf
