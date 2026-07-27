#pragma once
#include <mupdf/fitz.h>

namespace pdf {

struct PDFPage {
  std::size_t page_idx;
  fz_rect page_bounds;

  PDFPage(std::size_t idx, fz_rect bounds) : page_idx(idx), page_bounds(bounds) {}
};

} // namespace pdf
