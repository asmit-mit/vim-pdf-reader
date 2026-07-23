#include "pdf/pdf_page.h"

namespace pdf {

PDFPage::PDFPage(fz_context *ctx, fz_display_list *display_list)
    : ctx_(ctx), display_list_(display_list) {}

PDFPage::~PDFPage() {
  reset();
}

PDFPage::PDFPage(PDFPage &&other) noexcept {
  *this = std::move(other);
}

PDFPage &PDFPage::operator=(PDFPage &&other) noexcept {
  if (this != &other) {
    reset();

    ctx_ = other.ctx_;
    display_list_ = other.display_list_;

    other.ctx_ = nullptr;
    other.display_list_ = nullptr;
  }

  return *this;
}

fz_display_list *PDFPage::displayList() const {
  return display_list_;
}

void PDFPage::reset() {
  if (display_list_) {
    fz_drop_display_list(ctx_, display_list_);
    display_list_ = nullptr;
  }
}

} // namespace pdf
