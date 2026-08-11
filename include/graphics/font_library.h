#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <array>
#include <cstdint>
#include <functional>
#include <unordered_map>

namespace graphics {

enum FontType : int {
  Latin = 0,
  Emoji = 1,
  CJK = 2,
};

class FontLibrary {
public:
  FontLibrary();

  FontLibrary(const FontLibrary &) = delete;
  FontLibrary &operator=(const FontLibrary &) = delete;

  FontLibrary(FontLibrary &&) = delete;
  FontLibrary &operator=(FontLibrary &&) = delete;

  ~FontLibrary();

  void tryLoadFont(FontType type, const char *path);
  FontType getFontTypeForCodepoint(uint32_t codepoint) const;

  FT_Face getFontFace(FontType type, uint32_t pixel_size) const;

  bool empty() const;
  std::size_t size() const;

private:
  struct FontKey {
    FontType type;
    uint32_t size;

    bool operator==(const FontKey &other) const {
      return type == other.type && size == other.size;
    }
  };

  struct FontKeyHash {
    std::size_t operator()(const FontKey &key) const {
      std::size_t seed = 0;

      auto hashCombine = [&seed](std::size_t value) {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      };

      hashCombine(std::hash<int>{}(static_cast<int>(key.type)));
      hashCombine(std::hash<uint32_t>{}(key.size));

      return seed;
    }
  };

private:
  void destroy();

  FT_Library library_;

  mutable std::unordered_map<FontKey, FT_Face, FontKeyHash> cache_;
  std::array<FT_Face, 3> fonts_;
  std::array<const char *, 3> fonts_paths_;

  bool is_empty_;
};

} // namespace graphics
