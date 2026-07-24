#pragma once

#include <SFML/Graphics.hpp>

#include "pdf/pdf_document.h"
#include "pdf/pdf_page_display_list.h"
#include "utils/lru_cache.h"

namespace pdf {

using PageDisplayListCache = utils::LRUCache<std::size_t, pdf::PDFPageDisplayList>;

class PDFRenderer {
public:
  explicit PDFRenderer(PDFDocument &document);

  const sf::Texture &render(std::size_t page_idx, float zoom = 1.f, int rotate = 0);
  sf::Vector2u getPageSize(std::size_t page_idx, float zoom = 1.f, int rotate = 0);
  void clearCache();

private:
  fz_display_list *getPageDisplayList(fz_context *ctx, fz_document *doc, std::size_t page_idx);
  fz_display_list *getOrLoadDisplayList(std::size_t page_idx);
  fz_irect getTargetBBox(fz_context *ctx, fz_display_list *list, float zoom, int rot);
  fz_pixmap *getPixmapFromDisplayList(fz_context *ctx, fz_display_list *list, float zoom, int rot);
  void rastarizeToBitmap(fz_context *ctx, fz_pixmap *pix);

private:
  std::vector<std::uint8_t> rgba_;
  sf::Texture texture_;

  PDFDocument &document_;
  PageDisplayListCache page_display_list_cache_;
};

} // namespace pdf
