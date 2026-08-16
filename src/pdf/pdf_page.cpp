#include "pdf/pdf_page.h"

#include <algorithm>
#include <functional>

namespace pdf {

static char32_t normalizeChar(char32_t cp) {
  if (cp >= U'A' && cp <= U'Z')
    return cp + (U'a' - U'A');
  return cp;
}

static void appendSTextPage(
    std::u32string &text,
    std::u32string &text_normalized,
    std::vector<fz_rect> &rects,
    fz_stext_page *page_text
) {
  for (fz_stext_block *block = page_text->first_block; block; block = block->next) {
    if (block->type != FZ_STEXT_BLOCK_TEXT)
      continue;

    for (fz_stext_line *line = block->u.t.first_line; line; line = line->next) {
      for (fz_stext_char *ch = line->first_char; ch; ch = ch->next) {
        const auto cp = static_cast<char32_t>(ch->c);
        text.push_back(cp);
        text_normalized.push_back(normalizeChar(cp));
        rects.push_back(fz_rect_from_quad(ch->quad));
      }

      text.push_back(U'\n');
      text_normalized.push_back(U'\n');
      rects.push_back({});
    }

    text.push_back(U'\n');
    text_normalized.push_back(U'\n');
    rects.push_back({});
  }
}

PDFPage::PDFPage(std::size_t idx, fz_rect bounds) : page_idx(idx), page_bounds(bounds) {}

PDFPage::PDFPage(PDFPage &&other) noexcept
    : page_idx(other.page_idx), page_bounds(other.page_bounds), text(std::move(other.text)),
      text_normalized(std::move(other.text_normalized)), glyphs(std::move(other.glyphs)),
      content_loaded(other.content_loaded) {}

PDFPage &PDFPage::operator=(PDFPage &&other) noexcept {
  if (this == &other)
    return *this;
  std::scoped_lock lock(mutex_, other.mutex_);
  page_idx = other.page_idx;
  page_bounds = other.page_bounds;
  text = std::move(other.text);
  text_normalized = std::move(other.text_normalized);
  glyphs = std::move(other.glyphs);
  content_loaded = other.content_loaded;
  return *this;
}

void PDFPage::loadContent(fz_stext_page *page_text) {
  if (!page_text)
    return;

  std::lock_guard lock(mutex_);
  if (content_loaded)
    return;

  appendSTextPage(text, text_normalized, glyphs, page_text);
  content_loaded = true;
}

bool PDFPage::isContentLoaded() const {
  std::lock_guard lock(mutex_);
  return content_loaded;
}

std::vector<fz_rect> PDFPage::searchText(const std::u32string &pattern) const {
  std::vector<fz_rect> results;
  if (pattern.empty())
    return results;

  std::u32string normalized_pattern;
  normalized_pattern.reserve(pattern.size());
  for (char32_t cp : pattern)
    normalized_pattern.push_back(normalizeChar(cp));

  std::lock_guard lock(mutex_);

  if (!content_loaded || normalized_pattern.size() > text_normalized.size())
    return results;

  const std::boyer_moore_horspool_searcher
      searcher(normalized_pattern.begin(), normalized_pattern.end());

  auto it = text_normalized.begin();
  while (true) {
    it = std::search(it, text_normalized.end(), searcher);
    if (it == text_normalized.end())
      break;

    const std::size_t pos = static_cast<std::size_t>(it - text_normalized.begin());

    fz_rect merged = glyphs[pos];
    for (std::size_t i = pos + 1; i < pos + normalized_pattern.size(); ++i)
      merged = fz_union_rect(merged, glyphs[i]);
    results.push_back(merged);

    std::advance(it, normalized_pattern.size());
  }

  return results;
}

} // namespace pdf
