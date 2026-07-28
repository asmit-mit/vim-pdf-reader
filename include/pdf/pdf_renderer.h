#pragma once

#include <SFML/Graphics.hpp>
#include <mupdf/fitz.h>

#include <mutex>

#include "utils/lru_cache.h"

namespace pdf {

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

class PDFPageDisplayList {
public:
  PDFPageDisplayList() = default;
  PDFPageDisplayList(fz_context *ctx, fz_display_list *display_list);
  ~PDFPageDisplayList();
  PDFPageDisplayList(const PDFPageDisplayList &) = delete;
  PDFPageDisplayList &operator=(const PDFPageDisplayList &) = delete;
  PDFPageDisplayList(PDFPageDisplayList &&other) noexcept;
  PDFPageDisplayList &operator=(PDFPageDisplayList &&other) noexcept;

  fz_display_list *displayList() const;

private:
  void reset();

private:
  fz_context *ctx_ = nullptr;
  fz_display_list *display_list_ = nullptr;
};

class PDFRenderer {
public:
  explicit PDFRenderer();

  const sf::Image render(fz_context *ctx, fz_document *doc, const PDFRenderKey &key);
  sf::Vector2u getPageSize(fz_rect bounds, const PDFRenderKey &key);
  void clearCache();

private:
  fz_display_list *getPageDisplayList(std::size_t page_idx, fz_context *ctx, fz_document *doc);
  fz_display_list *getOrLoadDisplayList(std::size_t page_idx, fz_context *ctx, fz_document *doc);
  fz_pixmap *getPixmapFromDisplayList(fz_context *ctx, fz_display_list *list, float zoom, int rot);
  sf::Image rastarizeToBitmap(fz_context *ctx, fz_pixmap *pix);

private:
  using PDFPageDisplayListCache = utils::LRUCache<std::size_t, pdf::PDFPageDisplayList>;

  PDFPageDisplayListCache cache_;
  std::mutex display_mutex_;
};

} // namespace pdf
