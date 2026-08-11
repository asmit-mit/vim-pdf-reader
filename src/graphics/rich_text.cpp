#include "graphics/rich_text.h"
#include "graphics/font_library.h"
#include <print>

#include <iostream>
#include <stdexcept>

namespace graphics {

RichText::RichText(const graphics::FontLibrary &font_lib, uint32_t character_size)
    : font_lib_(font_lib), hb_buffer_(hb_buffer_create()), hb_fonts_{}, size_{0.f, 0.f},
      pixel_size_(character_size), text_color_(sf::Color::White) {
  if (!hb_buffer_allocation_successful(hb_buffer_))
    throw std::runtime_error("Failed to create hb_buffer");

  for (std::size_t i = 0; i < hb_fonts_.size(); ++i) {
    FontType type = static_cast<FontType>(i);
    FT_Face face = font_lib_.getFontFace(type, pixel_size_);
    if (!face)
      continue;

    hb_fonts_[i] = hb_ft_font_create_referenced(face);
    if (!hb_fonts_[i])
      throw std::runtime_error("Failed to create hb_font_t");
  }
}

RichText::~RichText() {
  for (hb_font_t *f : hb_fonts_) {
    if (f)
      hb_font_destroy(f);
  }
  if (hb_buffer_)
    hb_buffer_destroy(hb_buffer_);
}

void RichText::setString(const std::u32string &text) {
  sprites_.clear();
  shaped_glyphs_.clear();
  size_ = {0.f, 0.f};

  if (font_lib_.empty()) {
    std::cerr << "RichText::setString: no fonts loaded\n";
    return;
  }

  for (std::size_t i = 0; i < hb_fonts_.size(); ++i) {
    if (!hb_fonts_[i]) {
      FT_Face face = font_lib_.getFontFace(static_cast<FontType>(i), pixel_size_);
      if (!face)
        continue;
      hb_fonts_[i] = hb_ft_font_create_referenced(face);
    }
  }

  shapeAndCache(text);
}

void RichText::setColor(sf::Color color) {
  text_color_ = color;
}

void RichText::setCharacterSize(uint32_t size) {
  if (pixel_size_ == size)
    return;

  pixel_size_ = size;
  glyph_cache_.clear();

  for (std::size_t i = 0; i < hb_fonts_.size(); ++i) {
    FontType type = static_cast<FontType>(i);
    FT_Face face = font_lib_.getFontFace(type, pixel_size_);
    if (!face)
      continue;

    if (hb_fonts_[i])
      hb_font_destroy(hb_fonts_[i]);

    hb_fonts_[i] = hb_ft_font_create_referenced(face);
    if (!hb_fonts_[i])
      throw std::runtime_error("Failed to recreate hb_font_t");
  }
}

sf::Vector2f RichText::getSize() const {
  return size_;
}

const std::vector<ShapedGlyph> &RichText::getShapedGlyphs() const {
  return shaped_glyphs_;
}

void RichText::draw(sf::RenderTarget &target, sf::RenderStates states) const {
  sf::RenderStates local = states;
  local.transform *= getTransform();
  for (const sf::Sprite &sprite : sprites_)
    target.draw(sprite, local);
}

void RichText::createTextRuns(const std::u32string &text, std::vector<Run> &runs) {
  if (font_lib_.empty() || text.empty())
    return;

  uint32_t first = static_cast<uint32_t>(text[0]);
  graphics::FontType type = font_lib_.getFontTypeForCodepoint(first);
  std::u32string curr(1, text[0]);

  for (std::size_t i = 1; i < text.size(); i++) {
    uint32_t codepoint = static_cast<uint32_t>(text[i]);
    if (codepoint == 0xFE0F || codepoint == 0x200D) {
      curr += text[i];
      continue;
    }

    graphics::FontType new_type = font_lib_.getFontTypeForCodepoint(codepoint);
    if (new_type == type) {
      curr += text[i];
      continue;
    }

    runs.push_back({curr, type});

    type = new_type;
    curr.assign(1, text[i]);
  }

  if (!curr.empty())
    runs.push_back({curr, type});
}

const CachedGlyph *RichText::getOrRenderGlyph(FontType font_type, uint32_t glyph_idx) {
  GlyphKey key{font_type, glyph_idx, pixel_size_};

  auto it = glyph_cache_.find(key);
  if (it != glyph_cache_.end())
    return &it->second;

  FT_Face face = font_lib_.getFontFace(font_type, pixel_size_);
  if (!face)
    return nullptr;

  if (FT_Load_Glyph(face, glyph_idx, FT_LOAD_COLOR | FT_LOAD_RENDER) != 0)
    return nullptr;

  FT_GlyphSlot slot = face->glyph;
  FT_Bitmap &bitmap = slot->bitmap;

  CachedGlyph glyph;
  glyph.bearing_x = slot->bitmap_left;
  glyph.bearing_y = slot->bitmap_top;
  glyph.advance = slot->advance.x >> 6;
  glyph.is_color = (bitmap.pixel_mode == FT_PIXEL_MODE_BGRA);

  unsigned int width = bitmap.width;
  unsigned int height = bitmap.rows;

  if (width == 0 || height == 0) {
    auto [ins, _] = glyph_cache_.emplace(key, std::move(glyph));
    return &ins->second;
  }

  sf::Image img({width, height}, sf::Color::Transparent);
  const uint8_t *src = bitmap.buffer;
  int pitch = std::abs(bitmap.pitch);

  if (bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
    for (unsigned int y = 0; y < height; ++y) {
      const uint8_t *row = src + static_cast<std::ptrdiff_t>(y) * pitch;
      for (unsigned int x = 0; x < width; ++x) {
        uint8_t b = row[x * 4 + 0];
        uint8_t g = row[x * 4 + 1];
        uint8_t r = row[x * 4 + 2];
        uint8_t a = row[x * 4 + 3];

        if (a != 0 && a != 255) {
          r = static_cast<uint8_t>(std::min(255u, static_cast<uint32_t>(r) * 255u / a));
          g = static_cast<uint8_t>(std::min(255u, static_cast<uint32_t>(g) * 255u / a));
          b = static_cast<uint8_t>(std::min(255u, static_cast<uint32_t>(b) * 255u / a));
        }

        img.setPixel({x, y}, sf::Color(r, g, b, a));
      }
    }

  } else if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
    for (unsigned int y = 0; y < height; ++y) {
      const uint8_t *row = src + static_cast<std::ptrdiff_t>(y) * pitch;
      for (unsigned int x = 0; x < width; ++x)
        img.setPixel({x, y}, sf::Color(255, 255, 255, row[x]));
    }

  } else if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
    for (unsigned int y = 0; y < height; ++y) {
      const uint8_t *row = src + static_cast<std::ptrdiff_t>(y) * pitch;
      for (unsigned int x = 0; x < width; ++x) {
        uint8_t alpha = (row[x / 8] & (0x80u >> (x & 7u))) ? 255u : 0u;
        img.setPixel({x, y}, sf::Color(255, 255, 255, alpha));
      }
    }

  } else {
    std::cerr << "Warning: unsupported pixel mode " << bitmap.pixel_mode << " for glyph "
              << glyph_idx << "\n";
    auto [ins, _] = glyph_cache_.emplace(key, std::move(glyph));
    return &ins->second;
  }

  if (!glyph.texture.loadFromImage(img))
    std::cerr << "Warning: failed to upload glyph " << glyph_idx << " to GPU\n";
  else
    glyph.texture.setSmooth(true);

  if (font_type == Emoji && glyph.is_color) {
    float scale = static_cast<float>(pixel_size_) / static_cast<float>(height);
    glyph.texture.setSmooth(true);
    glyph.bearing_x = static_cast<int>(glyph.bearing_x * scale);
    glyph.bearing_y = static_cast<int>(glyph.bearing_y * scale);
    glyph.advance = static_cast<long>(pixel_size_);
    glyph.scale = scale;
  }

  auto [ins, _] = glyph_cache_.emplace(key, std::move(glyph));
  return &ins->second;
}

void RichText::processRuns(const std::vector<Run> &runs) {
  float pen_x = 0.f;
  float pen_y = 0.f;

  FT_Face latin_face = font_lib_.getFontFace(graphics::FontType::Latin, pixel_size_);
  if (latin_face)
    pen_y = static_cast<float>(latin_face->size->metrics.ascender >> 6);

  float line_height = pen_y;

  for (const Run &run : runs) {
    hb_font_t *hb_font = hb_fonts_[run.font_type];
    if (!hb_font)
      continue;

    hb_buffer_reset(hb_buffer_);
    hb_buffer_set_direction(hb_buffer_, HB_DIRECTION_LTR);
    hb_buffer_set_script(hb_buffer_, HB_SCRIPT_COMMON);
    hb_buffer_set_language(hb_buffer_, hb_language_from_string("en", -1));

    hb_buffer_add_utf32(
        hb_buffer_,
        reinterpret_cast<const uint32_t *>(run.codepoints.data()),
        static_cast<int>(run.codepoints.size()),
        0,
        static_cast<int>(run.codepoints.size())
    );

    hb_shape(hb_font, hb_buffer_, nullptr, 0);

    unsigned int count = 0;
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer_, &count);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer_, &count);

    for (unsigned int i = 0; i < count; ++i) {
      uint32_t glyph_idx = info[i].codepoint;

      float x_offset = static_cast<float>(pos[i].x_offset >> 6);
      float y_offset = static_cast<float>(pos[i].y_offset >> 6);
      float x_adv = static_cast<float>(pos[i].x_advance >> 6);

      const CachedGlyph *glyph = getOrRenderGlyph(run.font_type, glyph_idx);
      if (glyph) {
        sf::Sprite sprite(glyph->texture);
        sf::Vector2f glyph_pos{
            pen_x + x_offset + static_cast<float>(glyph->bearing_x),
            pen_y - y_offset - static_cast<float>(glyph->bearing_y)
        };
        sprite.setPosition(glyph_pos);
        sprite.setColor(glyph->is_color ? sf::Color::White : text_color_);
        sprite.setScale({glyph->scale, glyph->scale});

        sprites_.push_back(sprite);
      }

      long shaped_adv = (glyph && glyph->is_color) ? glyph->advance : static_cast<long>(x_adv);
      shaped_glyphs_.push_back({{pen_x, pen_y}, shaped_adv});
      pen_x += (glyph && glyph->is_color) ? static_cast<float>(glyph->advance) : x_adv;
    }
  }

  size_.x = pen_x;
  size_.y = line_height;
}

void RichText::shapeAndCache(const std::u32string &text) {
  if (font_lib_.empty() || text.empty())
    return;

  std::vector<Run> runs;
  createTextRuns(text, runs);
  processRuns(runs);
}

} // namespace graphics
