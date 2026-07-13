#include "pdf/pdf_renderer.h"

#include <stdexcept>

namespace pdf {

PDFRenderer::PDFRenderer(PDFDocument &document) : document_(document) {}

sf::Texture PDFRenderer::render(std::size_t page_idx, float zoom) {
  auto *ctx = document_.getCtx();
  auto *doc = document_.getDoc();

  if (page_idx >= document_.size())
    throw std::out_of_range("Page index out of range");

  fz_page *page = nullptr;
  fz_pixmap *pixmap = nullptr;
  fz_device *device = nullptr;
  sf::Texture texture;

  fz_try(ctx) {
    page = fz_load_page(ctx, doc, static_cast<int>(page_idx));

    fz_matrix matrix = fz_scale(zoom, zoom);
    fz_rect bounds = fz_bound_page(ctx, page);
    bounds = fz_transform_rect(bounds, matrix);

    fz_irect bbox = fz_round_rect(bounds);
    pixmap = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), bbox, nullptr, 1);
    fz_clear_pixmap_with_value(ctx, pixmap, 255);

    device = fz_new_draw_device(ctx, matrix, pixmap);
    fz_run_page(ctx, page, device, fz_identity, nullptr);
    fz_close_device(ctx, device);

    if (!texture.resize(
            {static_cast<unsigned int>(fz_pixmap_width(ctx, pixmap)),
             static_cast<unsigned int>(fz_pixmap_height(ctx, pixmap))}
        )) {
      throw std::runtime_error("Failed to create texture");
    }
    texture.update(fz_pixmap_samples(ctx, pixmap));
  }
  fz_always(ctx) {
    if (device)
      fz_drop_device(ctx, device);
    if (pixmap)
      fz_drop_pixmap(ctx, pixmap);
    if (page)
      fz_drop_page(ctx, page);
  }
  fz_catch(ctx) {
    throw std::runtime_error("Failed to render PDF page");
  }
  return texture;
}

} // namespace pdf
