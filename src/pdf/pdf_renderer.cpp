#include <stdexcept>

#include "pdf/pdf_renderer.h"
#include "utils/settings.h"

namespace pdf {

PDFPageDisplayList::PDFPageDisplayList(fz_context *ctx, fz_display_list *display_list)
    : ctx_(ctx), display_list_(display_list) {}

PDFPageDisplayList::~PDFPageDisplayList() {
  reset();
}

PDFPageDisplayList::PDFPageDisplayList(PDFPageDisplayList &&other) noexcept {
  *this = std::move(other);
}

PDFPageDisplayList &PDFPageDisplayList::operator=(PDFPageDisplayList &&other) noexcept {
  if (this != &other) {
    reset();

    ctx_ = other.ctx_;
    display_list_ = other.display_list_;

    other.ctx_ = nullptr;
    other.display_list_ = nullptr;
  }

  return *this;
}

fz_display_list *PDFPageDisplayList::displayList() const {
  return display_list_;
}

void PDFPageDisplayList::reset() {
  if (display_list_) {
    fz_drop_display_list(ctx_, display_list_);
    display_list_ = nullptr;
  }
}

PDFRenderer::PDFRenderer() : cache_(settings::display_list_cache_size_) {}

const sf::Image PDFRenderer::render(fz_context *ctx, fz_document *doc, const PDFRenderKey &key) {
  fz_display_list *list = getOrLoadDisplayList(key.page_idx, ctx, doc);
  fz_pixmap *pix = getPixmapFromDisplayList(ctx, list, key.zoom, key.rotate);
  return rastarizeToBitmap(ctx, pix);
}

sf::Vector2u PDFRenderer::getPageSize(fz_rect bounds, const PDFRenderKey &key) {
  fz_matrix ctm = fz_scale(key.zoom, key.zoom);
  ctm = fz_pre_rotate(ctm, key.rotate * 90.f);

  fz_rect transformed = fz_transform_rect(bounds, ctm);
  fz_irect bbox = fz_round_rect(transformed);
  return {static_cast<unsigned int>(bbox.x1 - bbox.x0), static_cast<unsigned int>(bbox.y1 - bbox.y0)};
}

void PDFRenderer::clearCache() {
  std::lock_guard lock(display_mutex_);
  cache_.clear();
}

fz_display_list *
PDFRenderer::getOrLoadDisplayList(std::size_t page_idx, fz_context *ctx, fz_document *doc) {
  std::lock_guard lock(display_mutex_);

  if (!cache_.contains(page_idx)) {
    fz_display_list *list = getPageDisplayList(page_idx, ctx, doc);
    cache_.put(page_idx, pdf::PDFPageDisplayList(ctx, list));
    return list;
  }

  return cache_.get(page_idx)->displayList();
}

fz_display_list *
PDFRenderer::getPageDisplayList(std::size_t page_idx, fz_context *ctx, fz_document *doc) {
  fz_page *page = nullptr;

  fz_try(ctx) page = fz_load_page(ctx, doc, page_idx);
  fz_catch(ctx) throw std::runtime_error("Failed to load page");

  fz_rect bounds = fz_bound_page(ctx, page);
  fz_display_list *list = fz_new_display_list(ctx, bounds);
  fz_device *dev = fz_new_list_device(ctx, list);

  fz_try(ctx) fz_run_page(ctx, page, dev, fz_identity, nullptr);
  fz_always(ctx) {
    fz_close_device(ctx, dev);
    fz_drop_device(ctx, dev);
    fz_drop_page(ctx, page);
  }
  fz_catch(ctx) {
    fz_drop_display_list(ctx, list);
    throw std::runtime_error("Failed to build display list");
  }

  return list;
}

fz_pixmap *
PDFRenderer::getPixmapFromDisplayList(fz_context *ctx, fz_display_list *list, float zoom, int rot) {
  fz_matrix ctm = fz_scale(zoom, zoom);
  ctm = fz_pre_rotate(ctm, rot * 90.f);

  fz_pixmap *pix = nullptr;
  fz_device *dev = nullptr;

  fz_rect bounds = fz_bound_display_list(ctx, list);
  fz_rect transformed = fz_transform_rect(bounds, ctm);
  fz_irect bbox = fz_round_rect(transformed);

  fz_try(ctx) {
    pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), bbox, nullptr, 0);
    fz_clear_pixmap_with_value(ctx, pix, 0xFF);

    dev = fz_new_draw_device(ctx, fz_identity, pix);
    fz_run_display_list(ctx, list, dev, ctm, transformed, nullptr);
    fz_close_device(ctx, dev);
  }
  fz_always(ctx) {
    if (dev)
      fz_drop_device(ctx, dev);
  }
  fz_catch(ctx) {
    if (pix)
      fz_drop_pixmap(ctx, pix);
    throw std::runtime_error("Failed to render page");
  }

  return pix;
}

sf::Image PDFRenderer::rastarizeToBitmap(fz_context *ctx, fz_pixmap *pix) {
  const int width = fz_pixmap_width(ctx, pix);
  const int height = fz_pixmap_height(ctx, pix);
  const int stride = fz_pixmap_stride(ctx, pix);
  const unsigned char *samples = fz_pixmap_samples(ctx, pix);

  std::vector<uint8_t> rgba(width * height * 4);
  for (int y = 0; y < height; ++y) {
    const unsigned char *src = samples + y * stride;
    for (int x = 0; x < width; ++x) {
      rgba[4 * (y * width + x) + 0] = src[3 * x + 0];
      rgba[4 * (y * width + x) + 1] = src[3 * x + 1];
      rgba[4 * (y * width + x) + 2] = src[3 * x + 2];
      rgba[4 * (y * width + x) + 3] = 255;
    }
  }

  sf::Image image({static_cast<unsigned>(width), static_cast<unsigned>(height)}, rgba.data());

  fz_drop_pixmap(ctx, pix);

  return image;
}

} // namespace pdf
