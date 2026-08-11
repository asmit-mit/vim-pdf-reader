#pragma once

#include <SFML/Graphics.hpp>

#include <ft2build.h>
#include <hb-ft.h>
#include <hb.h>

#include "graphics/font_library.h"

#include FT_FREETYPE_H
#include FT_BITMAP_H

namespace graphics {

struct GlyphKey {
  FontType face;
  uint32_t glyph_idx;
  uint32_t pixel_size;

  bool operator==(const GlyphKey &o) const noexcept {
    return face == o.face && glyph_idx == o.glyph_idx && pixel_size == o.pixel_size;
  }
};

struct GlyphKeyHash {
  std::size_t operator()(const GlyphKey &k) const noexcept {
    std::size_t h = std::hash<int>{}(k.face);
    h ^= std::hash<uint32_t>{}(k.glyph_idx) + 0x9e3779b9u + (h << 6) + (h >> 2);
    h ^= std::hash<uint32_t>{}(k.pixel_size) + 0x9e3779b9u + (h << 6) + (h >> 2);
    return h;
  }
};

struct CachedGlyph {
  sf::Texture texture;
  int bearing_x;
  int bearing_y;
  long advance;
  bool is_color;
  float scale = 1.f;
};

struct ShapedGlyph {
  sf::Vector2f position;
  long advance;
};

class RichText : public sf::Drawable, public sf::Transformable {
public:
  RichText(const graphics::FontLibrary &font_lib, uint32_t character_size = 30);

  RichText(const RichText &) = delete;
  RichText &operator=(const RichText &) = delete;
  RichText(RichText &&) = default;

  ~RichText();

  void setString(const std::u32string &text);
  void setColor(sf::Color color);
  void setCharacterSize(uint32_t size);

  sf::Vector2f getSize() const;
  const std::vector<ShapedGlyph> &getShapedGlyphs() const;

private:
  struct Run {
    std::u32string codepoints;
    FontType font_type;
  };

private:
  void createTextRuns(const std::u32string &text, std::vector<Run> &runs);
  void processRuns(const std::vector<Run> &runs);
  const CachedGlyph *getOrRenderGlyph(FontType font_type, uint32_t glyph_idx);
  void shapeAndCache(const std::u32string &text);

  void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

private:
  const graphics::FontLibrary &font_lib_;
  std::vector<ShapedGlyph> shaped_glyphs_;
  std::vector<sf::Sprite> sprites_;
  std::unordered_map<GlyphKey, CachedGlyph, GlyphKeyHash> glyph_cache_;

  hb_buffer_t *hb_buffer_;
  std::array<hb_font_t *, 3> hb_fonts_;

  sf::Vector2f size_;
  uint32_t pixel_size_;
  sf::Color text_color_;
};

} // namespace graphics
