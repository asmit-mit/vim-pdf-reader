// PDFPage.cpp

#include "pdf/pdf_page.h"

namespace pdf {

PDFPage::PDFPage(fz_context *ctx, std::size_t idx, fz_rect bounds, fz_stext_page *page_text)
    : page_idx(idx), page_bounds(bounds) {
  if (!page_text)
    return;

  fz_quad quad{};

  for (fz_stext_block *block = page_text->first_block; block; block = block->next) {
    if (block->type != FZ_STEXT_BLOCK_TEXT)
      continue;

    for (fz_stext_line *line = block->u.t.first_line; line; line = line->next) {
      for (fz_stext_char *ch = line->first_char; ch; ch = ch->next) {
        text.push_back(static_cast<char32_t>(ch->c));
        glyphs.push_back({static_cast<char32_t>(ch->c), ch->quad});
      }

      text.push_back(U'\n');
      glyphs.push_back({U'\n', quad});
    }

    text.push_back(U'\n');
    glyphs.push_back({U'\n', quad});
  }

  fz_drop_stext_page(ctx, page_text);
}

} // namespace pdf
