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
  page_count_ = 0;
}

void PDFDocument::openDocument(const std::string &filepath) {
  closeDocument();

  fz_try(ctx_) doc_ = fz_open_document(ctx_, filepath.c_str());
  fz_catch(ctx_) {
    throw std::runtime_error("Failed to open document");
  }

  fz_try(ctx_) page_count_ = fz_count_pages(ctx_, doc_);
  fz_catch(ctx_) {
    throw std::runtime_error("Failed to count number of pages");
  }
}

void PDFDocument::closeDocument() {
  if (doc_) {
    fz_drop_document(ctx_, doc_);
    doc_ = nullptr;
  }

  page_count_ = 0;
}

std::size_t PDFDocument::size() const {
  return page_count_;
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
