project('vim-pdf-reader', 'cpp', default_options: ['cpp_std=c++23'])
project_name = meson.project_name()

sfml_dep = dependency('sfml-graphics', required: true)
mupdf_dep = dependency('mupdf', required: true)
gtk_dep = dependency('gtk4', required: true)
harfbuzz_dep = dependency('harfbuzz', required: true)
freetype_dep = dependency('freetype2', required: true)

utf8cpp_includes = include_directories('thirdparty/utfcpp/source')

includes = include_directories('include')
sources = files(
  'src/app.cpp',
  'src/core/cmd_processor.cpp',
  'src/core/history_manager.cpp',
  'src/core/render_scheduler.cpp',
  'src/graphics/font_library.cpp',
  'src/graphics/glyph_atlas.cpp',
  'src/graphics/rich_text.cpp',
  'src/main.cpp',
  'src/pdf/pdf_document.cpp',
  'src/pdf/pdf_page.cpp',
  'src/pdf/pdf_renderer.cpp',
  'src/ui/cmdline.cpp',
  'src/ui/notifications.cpp',
  'src/ui/cursor.cpp',
  'src/ui/statusbar.cpp',
  'src/ui/page_view.cpp',
  'src/ui/pdf_view.cpp',
  'src/ui/textbox.cpp',
  'src/ui/completions.cpp',
  'src/ui/scrollwheel.cpp',
  'src/utils/trie.cpp'
)

executable(
  project_name,
  sources,
  include_directories: [includes, utf8cpp_includes],
  dependencies: [sfml_dep, mupdf_dep, gtk_dep, harfbuzz_dep, freetype_dep],
)
