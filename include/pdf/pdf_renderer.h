#pragma once

#include <SFML/Graphics.hpp>

#include "pdf/pdf_document.h"

namespace pdf {

class PDFRenderer {
public:
  explicit PDFRenderer(PDFDocument &document);

  const sf::Texture &render(std::size_t page_idx, float zoom = 1.f, int rotate = 0);

private:
  fz_display_list *getPageDisplayList(fz_context *ctx, fz_document *doc, std::size_t page_idx);
  fz_pixmap *getPixmapFromDisplayList(fz_context *ctx, fz_display_list *list, float zoom, int rot);
  void rastarizeToBitmap(fz_context *ctx, fz_pixmap *pix);

private:
  std::vector<std::uint8_t> rgba_;
  sf::Texture texture_;

  PDFDocument &document_;
  PageCache &page_cache_;
};

} // namespace pdf
