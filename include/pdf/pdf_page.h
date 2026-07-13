#pragma once

#include <mupdf/fitz.h>
#include <vector>

#include "pdf/pdf_document.h"

namespace pdf {

struct Glyph {
  char32_t codepoint;
  fz_quad quad;
};

struct Line {
  std::vector<Glyph> glyphs;
};

struct Block {
  std::vector<Line> lines;
};

class PDFPage {
public:
  PDFPage(PDFDocument &document, int page_idx);

  int index() const {
    return index_;
  }

  const std::vector<Block> &blocks() const {
    return blocks_;
  }

  const fz_rect &bounds() const {
    return bounds_;
  }

private:
  int index_;
  fz_rect bounds_{};

  std::vector<Block> blocks_;
};

} // namespace pdf
