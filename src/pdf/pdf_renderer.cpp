#include <stdexcept>

#include "pdf/pdf_renderer.h"
#include "utils/settings.h"

namespace pdf {

PDFRenderer::PDFRenderer(PDFDocument &document)
    : document_(document), page_display_list_cache_(settings::cache_size_) {}

fz_display_list *PDFRenderer::getOrLoadDisplayList(std::size_t page_idx) {
  auto *ctx = document_.getCtx();
  auto *doc = document_.getDoc();

  if (page_idx >= document_.size())
    throw std::out_of_range("Page index out of range");

  if (!page_display_list_cache_.contains(page_idx)) {
    fz_display_list *list = getPageDisplayList(ctx, doc, page_idx);
    page_display_list_cache_.put(page_idx, pdf::PDFPageDisplayList(ctx, list));
    return list;
  }

  return page_display_list_cache_.get(page_idx)->displayList();
}

const sf::Texture &PDFRenderer::render(std::size_t page_idx, float zoom, int rot) {
  auto *ctx = document_.getCtx();

  fz_display_list *list = getOrLoadDisplayList(page_idx);

  fz_pixmap *pix = getPixmapFromDisplayList(ctx, list, zoom, rot);
  rastarizeToBitmap(ctx, pix);

  return texture_;
}

sf::Vector2u PDFRenderer::getPageSize(std::size_t page_idx, float zoom, int rot) {
  auto *ctx = document_.getCtx();
  fz_display_list *list = getOrLoadDisplayList(page_idx);

  fz_irect bbox = getTargetBBox(ctx, list, zoom, rot);
  return {static_cast<unsigned int>(bbox.x1 - bbox.x0), static_cast<unsigned int>(bbox.y1 - bbox.y0)};
}

void PDFRenderer::clearCache() {
  page_display_list_cache_.clear();
}

fz_display_list *
PDFRenderer::getPageDisplayList(fz_context *ctx, fz_document *doc, std::size_t page_idx) {
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

fz_irect PDFRenderer::getTargetBBox(fz_context *ctx, fz_display_list *list, float zoom, int rot) {
  fz_matrix ctm = fz_scale(zoom, zoom);
  ctm = fz_pre_rotate(ctm, rot * 90.f);

  fz_rect bounds = fz_bound_display_list(ctx, list);
  fz_rect transformed = fz_transform_rect(bounds, ctm);
  return fz_round_rect(transformed);
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

void PDFRenderer::rastarizeToBitmap(fz_context *ctx, fz_pixmap *pix) {
  const int width = fz_pixmap_width(ctx, pix);
  const int height = fz_pixmap_height(ctx, pix);
  const int stride = fz_pixmap_stride(ctx, pix);
  const unsigned char *samples = fz_pixmap_samples(ctx, pix);

  rgba_.resize(width * height * 4);
  for (int y = 0; y < height; ++y) {
    const unsigned char *src = samples + y * stride;
    for (int x = 0; x < width; ++x) {
      rgba_[4 * (y * width + x) + 0] = src[3 * x + 0];
      rgba_[4 * (y * width + x) + 1] = src[3 * x + 1];
      rgba_[4 * (y * width + x) + 2] = src[3 * x + 2];
      rgba_[4 * (y * width + x) + 3] = 255;
    }
  }

  fz_drop_pixmap(ctx, pix);

  sf::Image
      image({static_cast<unsigned int>(width), static_cast<unsigned int>(height)}, rgba_.data());
  if (!texture_.loadFromImage(image))
    throw std::runtime_error("Failed to load texture from image");
}

} // namespace pdf
