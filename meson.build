project('vim-reader', 'cpp', default_options: ['cpp_std=c++23'])
project_name = meson.project_name()

sfml_dep = dependency('sfml-graphics', required: true)

includes = include_directories('include')
sources = files('src/main.cpp')

executable(project_name, sources, include_directories: includes, dependencies: sfml_dep)
