project('Xlz-cli-parser','cpp',
  version : '0.1',
  default_options:
  [
    'cpp_std=c++23',
    'cpp_eh=none',
    'cpp_rtti=false',
  ],
)

xlz_cli_inc = include_directories('include')

xlz_cli_dep = declare_dependency(
  include_directories : xlz_cli_inc,
)

install_headers(
  'include/xlz_cli/args.hpp',
  'include/xlz_cli/core/meta.hpp',
  'include/xlz_cli/core/parse.hpp',
  'include/xlz_cli/core/parse_impl.hpp',
  'include/xlz_cli/generic/generator.hpp',

  preserve_path : true,
)

if get_option('tests')
  subdir('tests')
endif
