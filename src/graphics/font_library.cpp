#include "graphics/font_library.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace graphics {

FontLibrary::FontLibrary() : library_(nullptr), fonts_{}, fonts_paths_{}, is_empty_(true) {
  if (FT_Init_FreeType(&library_) != 0)
    throw std::runtime_error("Failed to initialize font library");
}

FontLibrary::~FontLibrary() {
  destroy();
}

bool FontLibrary::empty() const {
  return is_empty_;
}

std::size_t FontLibrary::size() const {
  std::size_t count = 0;

  for (FT_Face font : fonts_) {
    if (font)
      ++count;
  }

  return count;
}

void FontLibrary::tryLoadFont(FontType type, const char *path) {
  FT_Face face = nullptr;

  if (FT_New_Face(library_, path, 0, &face) != 0) {
    if (type == FontType::Regular)
      throw std::runtime_error(std::string("Failed to load latin font: ") + path);
    std::cerr << "Warning: could not load " << path << " — skipping\n";
    return;
  }

  if (fonts_[type]) {
    FT_Done_Face(fonts_[type]);
  }

  fonts_[type] = face;
  fonts_paths_[type] = path;
  is_empty_ = false;
}

FontType FontLibrary::getFontTypeForCodepoint(uint32_t codepoint, FontType default_type) const {
  if (fonts_[default_type]) {
    if (FT_Get_Char_Index(fonts_[default_type], codepoint) != 0)
      return default_type;
  } else {
    if (fonts_[FontType::Regular] && FT_Get_Char_Index(fonts_[FontType::Regular], codepoint) != 0)
      return FontType::Regular;
  }

  if (fonts_[FontType::Emoji] && FT_Get_Char_Index(fonts_[FontType::Emoji], codepoint) != 0)
    return FontType::Emoji;
  if (fonts_[FontType::CJK] && FT_Get_Char_Index(fonts_[FontType::CJK], codepoint) != 0)
    return FontType::CJK;
  return FontType::Regular;
}

FT_Face FontLibrary::getFontFace(FontType type, uint32_t pixel_size) const {
  FontKey key{type, pixel_size};

  auto it = cache_.find(key);
  if (it != cache_.end())
    return it->second;

  const char *path = fonts_paths_[type];
  if (!path)
    return nullptr;

  FT_Face face = nullptr;

  if (FT_New_Face(library_, path, 0, &face) != 0)
    return nullptr;

  if (face->num_fixed_sizes > 0) {
    int best = 0;
    int best_diff = std::abs(face->available_sizes[0].height - (int)pixel_size);
    for (int i = 1; i < face->num_fixed_sizes; ++i) {
      int diff = std::abs(face->available_sizes[i].height - (int)pixel_size);
      if (diff < best_diff) {
        best_diff = diff;
        best = i;
      }
    }

    if (FT_Select_Size(face, best) != 0) {
      FT_Done_Face(face);
      return nullptr;
    }

  } else {
    if (FT_Set_Pixel_Sizes(face, 0, pixel_size) != 0) {
      FT_Done_Face(face);
      return nullptr;
    }
  }

  cache_.emplace(key, face);
  return face;
}

void FontLibrary::destroy() {
  for (auto &[key, face] : cache_) {
    if (face) {
      FT_Done_Face(face);
      face = nullptr;
    }
  }

  cache_.clear();

  for (FT_Face &font : fonts_) {
    if (font) {
      FT_Done_Face(font);
      font = nullptr;
    }
  }

  if (library_) {
    FT_Done_FreeType(library_);
    library_ = nullptr;
  }
}

} // namespace graphics
