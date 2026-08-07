#include <stdexcept>

#include "pdf/pdf_document.h"

namespace pdf {

PDFDocument::PDFDocument() {
  locks_.user = this;
  locks_.lock = &PDFDocument::lockMutex;
  locks_.unlock = &PDFDocument::unlockMutex;

  ctx_ = fz_new_context(nullptr, &locks_, FZ_STORE_DEFAULT);
  if (!ctx_)
    throw std::runtime_error("Failed to create MuPDF context");

  fz_try(ctx_) fz_register_document_handlers(ctx_);
  fz_catch(ctx_) {
    fz_drop_context(ctx_);
    ctx_ = nullptr;
    throw std::runtime_error("Failed to register document handlers");
  }

  doc_ = nullptr;
}

void PDFDocument::openDocument(const std::string &filepath) {
  closeDocument();

  fz_try(ctx_) doc_ = fz_open_document(ctx_, filepath.c_str());
  fz_catch(ctx_) {
    throw std::runtime_error("Failed to open document: " + filepath);
  }

  int page_count;
  fz_try(ctx_) page_count = fz_count_pages(ctx_, doc_);
  fz_catch(ctx_) {
    throw std::runtime_error("Failed to count number of pages");
  }

  int max_width = 0;
  page_with_max_width_ = 0;

  pages_.reserve(page_count);
  for (int i = 0; i < page_count; i++) {
    fz_page *page = fz_load_page(ctx_, doc_, i);
    fz_rect bounds = fz_bound_page(ctx_, page);
    // fz_stext_page *text = fz_new_stext_page_from_page(ctx_, page, NULL);

    int width = std::abs(bounds.x0 - bounds.x1);
    if (width > max_width) {
      page_with_max_width_ = i;
      max_width = width;
    }

    pages_.emplace_back(ctx_, i, bounds, nullptr);
  }
}

void PDFDocument::closeDocument() {
  if (doc_) {
    fz_drop_document(ctx_, doc_);
    doc_ = nullptr;
  }

  pages_.clear();
}

std::size_t PDFDocument::pageWithMaxWidth() const {
  if (!doc_)
    return 0;
  return page_with_max_width_;
}

PDFPage &PDFDocument::getPage(std::size_t page_idx) {
  return pages_[page_idx];
}

std::size_t PDFDocument::size() const {
  return pages_.size();
}

FzContextPtr PDFDocument::cloneContext() const {
  return FzContextPtr(fz_clone_context(ctx_), &fz_drop_context);
}

fz_document *PDFDocument::getDoc() const {
  return doc_;
}

PDFDocument::~PDFDocument() {
  closeDocument();

  if (ctx_) {
    fz_drop_context(ctx_);
    ctx_ = nullptr;
  }
}

void PDFDocument::lockMutex(void *user, int lock) {
  static_cast<PDFDocument *>(user)->mutexes_[lock].lock();
}

void PDFDocument::unlockMutex(void *user, int lock) {
  static_cast<PDFDocument *>(user)->mutexes_[lock].unlock();
}

} // namespace pdf
