#include "graphics/glyph_atlas.h"

#include <stdexcept>

namespace graphics {

GlyphAtlas::GlyphAtlas(unsigned int width, unsigned int height)
    : atlas_w_(width), atlas_h_(height), curr_page_{} {
  atlas_pages_.emplace_back();
  if (!atlas_pages_.back().texture.resize({atlas_w_, atlas_h_}))
    throw std::runtime_error("Failed to resize atlas texture");
}

void GlyphAtlas::addPage() {
  curr_page_++;
  if (curr_page_ >= atlas_pages_.size()) {
    atlas_pages_.emplace_back();
    if (!atlas_pages_.back().texture.resize({atlas_w_, atlas_h_}))
      throw std::runtime_error("Failed to resize atlas texture");
    return;
  }

  AtlasPage &page = atlas_pages_[curr_page_];
  page.cursor_x = 0;
  page.cursor_y = 0;
  page.row_h = 0;
  page.texture.update(sf::Image({atlas_w_, atlas_h_}, sf::Color::Transparent));
}

const sf::Texture &GlyphAtlas::texture(std::size_t page) const {
  return atlas_pages_.at(page).texture;
}

const AtlasGlyph *
GlyphAtlas::getOrPack(FontType font_type, uint32_t glyph_idx, uint32_t pixel_size, FT_Face face) {
  Key key{font_type, glyph_idx, pixel_size};

  auto it = cache_.find(key);
  if (it != cache_.end())
    return &it->second;

  if (!face)
    return nullptr;

  if (FT_Load_Glyph(face, glyph_idx, FT_LOAD_COLOR | FT_LOAD_RENDER) != 0)
    return nullptr;

  FT_GlyphSlot slot = face->glyph;
  FT_Bitmap &bitmap = slot->bitmap;

  AtlasGlyph glyph;
  glyph.bearing_x = slot->bitmap_left;
  glyph.bearing_y = slot->bitmap_top;
  glyph.advance = slot->advance.x >> 6;
  glyph.is_color = (bitmap.pixel_mode == FT_PIXEL_MODE_BGRA);

  unsigned int width = bitmap.width;
  unsigned int height = bitmap.rows;

  if (width == 0 || height == 0) {
    glyph.uv = {};
    glyph.page = curr_page_;
    auto [ins, _] = cache_.emplace(key, glyph);
    return &ins->second;
  }

  AtlasPage &curr_page = atlas_pages_[curr_page_];
  if (curr_page.cursor_x + width > atlas_w_) {
    curr_page.cursor_y += curr_page.row_h;
    curr_page.cursor_x = 0;
    curr_page.row_h = 0;
  }

  if (curr_page.cursor_y + height > atlas_h_)
    addPage();

  AtlasPage &curr = atlas_pages_[curr_page_];
  glyph.page = curr_page_;

  unsigned int cx = curr.cursor_x;
  unsigned int cy = curr.cursor_y;

  sf::Image staging({width, height}, sf::Color::Transparent);
  const uint8_t *src = bitmap.buffer;
  int pitch = std::abs(bitmap.pitch);

  if (bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
    for (unsigned int y = 0; y < height; ++y) {
      const uint8_t *row = src + static_cast<std::ptrdiff_t>(y) * pitch;
      for (unsigned int x = 0; x < width; ++x) {
        uint8_t b = row[x * 4 + 0], g = row[x * 4 + 1], r = row[x * 4 + 2], a = row[x * 4 + 3];
        if (a != 0 && a != 255) {
          r = static_cast<uint8_t>(std::min(255u, (uint32_t)r * 255u / a));
          g = static_cast<uint8_t>(std::min(255u, (uint32_t)g * 255u / a));
          b = static_cast<uint8_t>(std::min(255u, (uint32_t)b * 255u / a));
        }
        staging.setPixel({x, y}, sf::Color(r, g, b, a));
      }
    }
  } else if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
    for (unsigned int y = 0; y < height; ++y) {
      const uint8_t *row = src + static_cast<std::ptrdiff_t>(y) * pitch;
      for (unsigned int x = 0; x < width; ++x)
        staging.setPixel({x, y}, sf::Color(255, 255, 255, row[x]));
    }
  } else if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
    for (unsigned int y = 0; y < height; ++y) {
      const uint8_t *row = src + static_cast<std::ptrdiff_t>(y) * pitch;
      for (unsigned int x = 0; x < width; ++x) {
        uint8_t a = (row[x / 8] & (0x80u >> (x & 7u))) ? 255u : 0u;
        staging.setPixel({x, y}, sf::Color(255, 255, 255, a));
      }
    }
  }

  curr.texture.update(staging, {cx, cy});

  glyph.uv = sf::
      FloatRect({(float)cx / atlas_w_, (float)cy / atlas_h_}, {(float)width / atlas_w_, (float)height / atlas_h_});

  if (glyph.is_color) {
    float scale = static_cast<float>(pixel_size) / static_cast<float>(height);
    glyph.bearing_x = static_cast<int>(glyph.bearing_x * scale);
    glyph.bearing_y = static_cast<int>(glyph.bearing_y * scale);
    glyph.advance = static_cast<long>(pixel_size + 1);
    glyph.scale = scale;
  }

  curr.row_h = std::max(curr.row_h, height);
  curr.cursor_x = cx + width;

  auto [ins, _] = cache_.emplace(key, glyph);
  return &ins->second;
}

std::size_t GlyphAtlas::pageCount() const {
  return atlas_pages_.size();
}

std::size_t GlyphAtlas::width() {
  return atlas_w_;
}

std::size_t GlyphAtlas::height() {
  return atlas_h_;
}

void GlyphAtlas::clear() {
  cache_.clear();
  curr_page_ = 0;
}

} // namespace graphics
