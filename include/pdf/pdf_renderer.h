#pragma once

#include <SFML/Graphics.hpp>

#include "pdf/pdf_document.h"

namespace pdf {

class PDFRenderer {
public:
  explicit PDFRenderer(PDFDocument &document);

  sf::Texture render(std::size_t page_idx, float zoom = 1.f);

private:
  PDFDocument &document_;
};

} // namespace pdf
