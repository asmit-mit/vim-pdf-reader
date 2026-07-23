#pragma once

#include <mupdf/fitz.h>

namespace pdf {

class PDFPage {
public:
  PDFPage() = default;
  PDFPage(fz_context *ctx, fz_display_list *display_list);
  ~PDFPage();
  PDFPage(const PDFPage &) = delete;
  PDFPage &operator=(const PDFPage &) = delete;
  PDFPage(PDFPage &&other) noexcept;
  PDFPage &operator=(PDFPage &&other) noexcept;

  fz_display_list *displayList() const;

private:
  void reset();

private:
  fz_context *ctx_ = nullptr;
  fz_display_list *display_list_ = nullptr;
};

} // namespace pdf
