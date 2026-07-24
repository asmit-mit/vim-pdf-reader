#include "pdf/pdf_page_display_list.h"

namespace pdf {

PDFPageDisplayList::PDFPageDisplayList(fz_context *ctx, fz_display_list *display_list)
    : ctx_(ctx), display_list_(display_list) {}

PDFPageDisplayList::~PDFPageDisplayList() {
  reset();
}

PDFPageDisplayList::PDFPageDisplayList(PDFPageDisplayList &&other) noexcept {
  *this = std::move(other);
}

PDFPageDisplayList &PDFPageDisplayList::operator=(PDFPageDisplayList &&other) noexcept {
  if (this != &other) {
    reset();

    ctx_ = other.ctx_;
    display_list_ = other.display_list_;

    other.ctx_ = nullptr;
    other.display_list_ = nullptr;
  }

  return *this;
}

fz_display_list *PDFPageDisplayList::displayList() const {
  return display_list_;
}

void PDFPageDisplayList::reset() {
  if (display_list_) {
    fz_drop_display_list(ctx_, display_list_);
    display_list_ = nullptr;
  }
}

} // namespace pdf
