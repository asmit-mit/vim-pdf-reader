project('vim-reader', 'cpp', default_options: ['cpp_std=c++23'])
project_name = meson.project_name()

sfml_dep = dependency('sfml-graphics', required: true)
mupdf_dep = dependency('mupdf', required: true)

includes = include_directories('include')
sources = files(
  'src/app.cpp',
  'src/core/command_processor.cpp',
  'src/main.cpp',
  'src/pdf/pdf_document.cpp',
  'src/pdf/pdf_page.cpp',
  'src/pdf/pdf_renderer.cpp',
  'src/ui/cmdline.cpp',
  'src/ui/cursor.cpp',
  'src/ui/toolbar.cpp',
  'src/ui/pdf_view.cpp',
)

executable(
  project_name,
  sources,
  include_directories: includes,
  dependencies: [sfml_dep, mupdf_dep],
)
