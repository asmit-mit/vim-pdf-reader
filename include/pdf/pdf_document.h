#pragma once

#include <mupdf/fitz.h>
#include <string>

namespace pdf {

class PDFDocument {
public:
  explicit PDFDocument();
  ~PDFDocument();
  PDFDocument(const PDFDocument &) = delete;
  PDFDocument &operator=(const PDFDocument &) = delete;

  void openDocument(const std::string &filepath);
  void closeDocument();

  std::size_t size() const;
  fz_context *getCtx() const;
  fz_document *getDoc() const;

private:
  fz_context *ctx_;
  fz_document *doc_;

  std::size_t page_count_;
};

} // namespace pdf
