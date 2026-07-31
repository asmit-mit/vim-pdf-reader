project('vim-pdf-reader', 'cpp', default_options: ['cpp_std=c++23'])
project_name = meson.project_name()

sfml_dep = dependency('sfml-graphics', required: true)
mupdf_dep = dependency('mupdf', required: true)
gtk_dep = dependency('gtk4', required: true)

includes = include_directories('include')
sources = files(
  'src/app.cpp',
  'src/core/cmd_processor.cpp',
  'src/core/cmd_history.cpp',
  'src/core/render_scheduler.cpp',
  'src/main.cpp',
  'src/pdf/pdf_document.cpp',
  'src/pdf/pdf_renderer.cpp',
  'src/ui/cmdline.cpp',
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
  include_directories: includes,
  dependencies: [sfml_dep, mupdf_dep, gtk_dep],
)
