#pragma once

#include <SFML/Graphics.hpp>

#include <ft2build.h>
#include <unordered_map>
#include FT_FREETYPE_H

#include "graphics/font_library.h"

namespace graphics {

struct AtlasGlyph {
  std::size_t page;
  sf::FloatRect uv;

  int bearing_x;
  int bearing_y;
  long advance;

  bool is_color;
  float scale = 1.f;
};

class GlyphAtlas {
public:
  GlyphAtlas(unsigned int width = 1024, unsigned int height = 1024);

  const AtlasGlyph *
  getOrPack(FontType font_type, uint32_t glyph_idx, uint32_t pixel_size, FT_Face face);

  const sf::Texture &texture(std::size_t page) const;
  std::size_t pageCount() const;
  std::size_t width();
  std::size_t height();
  void clear();

private:
  struct Key {
    FontType font_type;
    uint32_t glyph_idx;
    uint32_t pixel_size;

    bool operator==(const Key &other) const noexcept {
      return font_type == other.font_type && glyph_idx == other.glyph_idx &&
             pixel_size == other.pixel_size;
    }
  };

  struct KeyHash {
    std::size_t operator()(const Key &key) const noexcept {
      std::size_t h = std::hash<int>{}(key.font_type);
      h ^= std::hash<uint32_t>{}(key.glyph_idx) + 0x9e3779b9u + (h << 6) + (h >> 2);
      h ^= std::hash<uint32_t>{}(key.pixel_size) + 0x9e3779b9u + (h << 6) + (h >> 2);
      return h;
    }
  };

  struct AtlasPage {
    sf::Texture texture;

    unsigned int cursor_x{};
    unsigned int cursor_y{};
    unsigned int row_h{};
  };

private:
  void addPage();

private:
  std::unordered_map<Key, AtlasGlyph, KeyHash> cache_;
  std::vector<AtlasPage> atlas_pages_;

  unsigned int atlas_w_, atlas_h_;
  std::size_t curr_page_;
};

} // namespace graphics
