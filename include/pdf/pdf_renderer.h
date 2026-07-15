#pragma once

#include <SFML/Graphics.hpp>

#include "pdf/pdf_document.h"

namespace pdf {

class PDFRenderer {
public:
  explicit PDFRenderer(PDFDocument &document);

  const sf::Texture &render(std::size_t page_idx, float zoom = 1.f, int rotate = 0);

private:
  sf::Texture texture_;

  PDFDocument &document_;
};

} // namespace pdf
