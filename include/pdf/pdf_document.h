#pragma once

#include <mupdf/fitz.h>

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "pdf/pdf_page.h"

namespace pdf {

using FzContextPtr = std::unique_ptr<fz_context, decltype(&fz_drop_context)>;

class PDFDocument {
public:
  PDFDocument();
  ~PDFDocument();

  PDFDocument(const PDFDocument &) = delete;
  PDFDocument &operator=(const PDFDocument &) = delete;

  void openDocument(const std::string &filepath);
  void closeDocument();
  PDFPage &getPage(std::size_t page_idx);

  std::size_t size() const;

  FzContextPtr cloneContext() const;
  fz_document *getDoc() const;

private:
  static void lockMutex(void *user, int lock);
  static void unlockMutex(void *user, int lock);

private:
  std::array<std::mutex, FZ_LOCK_MAX> mutexes_;
  fz_locks_context locks_;

  std::vector<pdf::PDFPage> pages_;

  fz_context *ctx_;
  fz_document *doc_;
};

} // namespace pdf
