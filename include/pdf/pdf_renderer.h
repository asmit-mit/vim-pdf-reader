#pragma once

#include <SFML/Graphics.hpp>
#include <mutex>

#include "pdf/pdf_page_display_list.h"
#include "utils/lru_cache.h"

namespace pdf {

using PDFPageDisplayListCache = utils::LRUCache<std::size_t, pdf::PDFPageDisplayList>;

struct PDFRenderKey {
  std::size_t page_idx;
  float zoom;
  int rotate;

  bool operator==(const PDFRenderKey &other) const {
    return page_idx == other.page_idx && zoom == other.zoom && rotate == other.rotate;
  }
};

struct PDFRenderKeyHash {
  std::size_t operator()(const PDFRenderKey &key) const {
    std::size_t seed = 0;

    auto hashCombine = [&seed](std::size_t value) {
      seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    };

    hashCombine(std::hash<std::size_t>{}(key.page_idx));
    hashCombine(std::hash<float>{}(key.zoom));
    hashCombine(std::hash<int>{}(key.rotate));

    return seed;
  }
};

class PDFRenderer {
public:
  explicit PDFRenderer();

  const sf::Image render(fz_context *ctx, fz_document *doc, const PDFRenderKey &key);
  sf::Vector2u getPageSize(fz_context *ctx, fz_document *doc, const PDFRenderKey &key);
  void clearCache();

private:
  fz_display_list *getPageDisplayList(std::size_t page_idx, fz_context *ctx, fz_document *doc);
  fz_display_list *getOrLoadDisplayList(std::size_t page_idx, fz_context *ctx, fz_document *doc);
  fz_irect getTargetBBox(fz_context *ctx, fz_display_list *list, float zoom, int rot);
  fz_pixmap *getPixmapFromDisplayList(fz_context *ctx, fz_display_list *list, float zoom, int rot);
  sf::Image rastarizeToBitmap(fz_context *ctx, fz_pixmap *pix);

private:
  PDFPageDisplayListCache cache_;
  std::mutex display_mutex_;
};

} // namespace pdf
