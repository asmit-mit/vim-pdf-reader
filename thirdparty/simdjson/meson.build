simdjson_includes = include_directories('.')

simdjson_lib = static_library(
  'simdjson',
  'simdjson.cpp',
  include_directories: simdjson_includes,
  override_options: ['cpp_std=c++17'],
  cpp_args: ['-w'],
)

simdjson_dep = declare_dependency(
  link_with: simdjson_lib,
  include_directories: simdjson_includes,
)
