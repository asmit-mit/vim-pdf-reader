#include "pdf/pdf_page.h"
#include "pdf/pdf_document.h"

namespace pdf {

PDFPage::PDFPage(PDFDocument &document, int page_idx) : index_(page_idx) {
  auto *ctx = document.getCtx();
  auto *doc = document.getDoc();

  fz_page *page = fz_load_page(ctx, doc, page_idx);

  bounds_ = fz_bound_page(ctx, page);

  fz_stext_options options{};
  fz_stext_page *text = fz_new_stext_page_from_page(ctx, page, &options);

  for (fz_stext_block *block = text->first_block; block; block = block->next) {
    if (block->type != FZ_STEXT_BLOCK_TEXT)
      continue;

    Block new_block;

    for (fz_stext_line *line = block->u.t.first_line; line; line = line->next) {
      Line new_line;

      for (fz_stext_char *ch = line->first_char; ch; ch = ch->next) {
        new_line.glyphs.push_back({static_cast<char32_t>(ch->c), ch->quad});
      }

      new_block.lines.push_back(std::move(new_line));
    }

    blocks_.push_back(std::move(new_block));
  }

  fz_drop_stext_page(ctx, text);
  fz_drop_page(ctx, page);
}

} // namespace pdf
