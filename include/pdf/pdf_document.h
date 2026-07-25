#pragma once

#include <array>
#include <memory>
#include <mupdf/fitz.h>
#include <mutex>
#include <string>

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

  std::size_t size() const;

  FzContextPtr cloneContext() const;
  fz_document *getDoc() const;

private:
  static void lockMutex(void *user, int lock);
  static void unlockMutex(void *user, int lock);

private:
  std::array<std::mutex, FZ_LOCK_MAX> mutexes_;
  fz_locks_context locks_;

  std::size_t page_count_;
  fz_context *ctx_;
  fz_document *doc_;
};

} // namespace pdf
