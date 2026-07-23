#pragma once

#include <mupdf/fitz.h>
#include <string>

#include "pdf/pdf_page.h"
#include "utils/lru_cache.h"

namespace pdf {

using PageCache = utils::LRUCache<std::size_t, pdf::PDFPage>;

class PDFDocument {
public:
  explicit PDFDocument();
  ~PDFDocument();
  PDFDocument(const PDFDocument &) = delete;
  PDFDocument &operator=(const PDFDocument &) = delete;

  void openDocument(const std::string &filepath);
  void closeDocument();

  PageCache &getCache();
  std::size_t size() const;
  fz_context *getCtx() const;
  fz_document *getDoc() const;

private:
  PageCache page_cache_;
  std::size_t page_count_;
  fz_context *ctx_;
  fz_document *doc_;
};

} // namespace pdf
