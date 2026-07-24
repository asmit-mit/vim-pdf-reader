#pragma once

#include <mupdf/fitz.h>

namespace pdf {

class PDFPageDisplayList {
public:
  PDFPageDisplayList() = default;
  PDFPageDisplayList(fz_context *ctx, fz_display_list *display_list);
  ~PDFPageDisplayList();
  PDFPageDisplayList(const PDFPageDisplayList &) = delete;
  PDFPageDisplayList &operator=(const PDFPageDisplayList &) = delete;
  PDFPageDisplayList(PDFPageDisplayList &&other) noexcept;
  PDFPageDisplayList &operator=(PDFPageDisplayList &&other) noexcept;

  fz_display_list *displayList() const;

private:
  void reset();

private:
  fz_context *ctx_ = nullptr;
  fz_display_list *display_list_ = nullptr;
};

} // namespace pdf
