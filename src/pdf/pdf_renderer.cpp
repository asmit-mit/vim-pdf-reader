#include <iostream>
#include <stdexcept>

#include "pdf/pdf_renderer.h"

namespace pdf {

PDFRenderer::PDFRenderer(PDFDocument &document) : document_(document) {}

const sf::Texture &PDFRenderer::render(std::size_t page_idx, float zoom, int rotate) {
  auto *ctx = document_.getCtx();
  auto *doc = document_.getDoc();

  if (page_idx >= document_.size())
    throw std::out_of_range("Page index out of range");

  fz_matrix ctm = fz_scale(zoom, zoom);
  ctm = fz_pre_rotate(ctm, rotate * 90.f);

  fz_pixmap *pix;
  fz_try(ctx) pix = fz_new_pixmap_from_page_number(ctx, doc, page_idx, ctm, fz_device_rgb(ctx), 0);
  fz_catch(ctx) {
    fprintf(stderr, "Failed to render page");
  }

  int width = fz_pixmap_width(ctx, pix);
  int height = fz_pixmap_height(ctx, pix);
  int stride = fz_pixmap_stride(ctx, pix);
  unsigned char *samples = fz_pixmap_samples(ctx, pix);

  std::vector<std::uint8_t> rgba(width * height * 4);

  for (int y = 0; y < height; ++y) {
    const unsigned char *src = samples + y * stride;

    for (int x = 0; x < width; ++x) {
      rgba[(y * width + x) * 4 + 0] = src[x * 3 + 0]; // R
      rgba[(y * width + x) * 4 + 1] = src[x * 3 + 1]; // G
      rgba[(y * width + x) * 4 + 2] = src[x * 3 + 2]; // B
      rgba[(y * width + x) * 4 + 3] = 255;            // A
    }
  }

  sf::Image image({static_cast<unsigned>(width), static_cast<unsigned>(height)}, rgba.data());

  // if (!image.saveToFile("mupdf_test.png")) {
  //   fz_drop_pixmap(ctx, pix);
  //   throw std::runtime_error("Failed to save image");
  // }

  if (!texture_.loadFromImage(image)) {
    fz_drop_pixmap(ctx, pix);
    throw std::runtime_error("Failed to load texture from image");
  }

  fz_drop_pixmap(ctx, pix);
  return texture_;
}

} // namespace pdf
