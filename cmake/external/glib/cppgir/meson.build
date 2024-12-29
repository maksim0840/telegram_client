project('cppgir',
  ['c', 'cpp'],
  meson_version : '>= 0.61',
  version : '2.0.0',
  default_options : [
    'warning_level=2',
    'cpp_std=c++17'
  ]
)

message('meson system only considers generator and includes',
  '\n\tsee CMake build for full build including examples')

compiler = meson.get_compiler('cpp')

foreach arg : ['-Wnon-virtual-dtor']
  if compiler.has_argument(arg)
    add_project_arguments(arg, language: 'cpp')
  endif
endforeach

# generator binary

# dependencies
boost_dep = dependency('boost', version : '>=1.58', required : true)

# fmtlib
fmtlib_dep = compiler.find_library('fmt', has_headers : ['fmt/format.h'])
# check C++20 format
has_format = compiler.compiles(files('cmake/cpp20_format.cpp'),
  args : ['-std=c++20'],
  name : 'std::format check'
)

# check and decide on which fmt to use
use_fmtlib = 'none'
build_fmt = get_option('build_fmt')
if build_fmt == 'auto'
  if fmtlib_dep.found()
    use_fmtlib = '1'
  elif has_format
    use_fmtlib = '0'
  endif
elif build_fmt == 'fmtlib' and fmtlib_dep.found()
  use_fmtlib = '1'
elif build_fmt == 'stdformat' and has_format
  use_fmtlib = '0'
endif
if use_fmtlib == 'none'
  error('no format library found')
endif

# required ignore file
fs = import('fs')

gi_ignore_file_dir = 'data'
gi_ignore_file = 'cppgir.ignore'
cppgir_ignore = fs.read('data/cppgir.ignore')
if host_machine.system() == 'windows'
  gi_ignore_file_platform = 'cppgir_win.ignore'
  cppgir_win_ignore = fs.read('data/cppgir_win.ignore')
  cppgir_unix_ignore = ''
else
  gi_ignore_file_platform = 'cppgir_unix.ignore'
  cppgir_unix_ignore = fs.read('data/cppgir_unix.ignore')
  cppgir_win_ignore = ''
endif
gi_install_full_datadir = \
  '@0@/@1@'.format(get_option('prefix'), get_option('datadir'))
gi_ignore_file_install_dir =  \
  '@0@/@1@'.format(gi_install_full_datadir, meson.project_name())

cppgir_sources = [
  'tools/cppgir.cpp',
  'tools/genbase.cpp', 'tools/genbase.hpp',
  'tools/genns.cpp', 'tools/genns.hpp',
  'tools/genutils.cpp', 'tools/genutils.hpp',
  'tools/function.cpp', 'tools/function.hpp',
  'tools/repository.cpp', 'tools/repository.hpp',
  'tools/common.hpp'
]

cppgir_deps = [boost_dep]
cppgir_overrides = []
cppgir_args = []

# adjust to options and situation
if use_fmtlib.to_int() > 0
  cppgir_deps += [fmtlib_dep]
else
  cppgir_overrides = ['cpp_std=c++20']
endif

if get_option('build_embed_ignore')
  # generate embedded ignore data
  conf_data = configuration_data()
  conf_data.set('CPPGIR_IGNORE', cppgir_ignore)
  conf_data.set('CPPGIR_UNIX_IGNORE', cppgir_unix_ignore)
  conf_data.set('CPPGIR_WIN_IGNORE', cppgir_win_ignore)
  configure_file(configuration : conf_data,
    input : 'tools/ignore.hpp.in', output : 'ignore.hpp')
else
  install_data(gi_ignore_file_dir / gi_ignore_file,
    gi_ignore_file_dir / gi_ignore_file_platform,
    install_dir : gi_ignore_file_install_dir)
  cppgir_args += \
    [f'-DDEFAULT_IGNORE_FILE=@gi_ignore_file_install_dir@/@gi_ignore_file@:@gi_ignore_file_install_dir@/@gi_ignore_file_platform@']
endif

# gir search path
if host_machine.system() != 'windows'
  # add fixed fallback search places
  gi_default_girpath = '/usr/@0@:/usr/local/@0@'.format(get_option('datadir'))
  cppgir_args += [
    f'-DDEFAULT_GIRPATH=@gi_default_girpath@',
    f'-DGI_DATA_DIR=@gi_install_full_datadir@/gir-1.0',
    '-DGI_DEF_DIR=/usr/share/gir-1.0'
  ]
  gir_dir = get_option('gir_dir')
  if gir_dir != ''
    cppgir_args += [f'-DGI_GIR_DIR=@gir_dir@']
  endif
endif

if get_option('link_stdfs')
  # some older gcc might sometimes (?) need this, even in c++17 mode
  # see issue #80
  fs_dep = compiler.find_library('stdc++fs')
  cppgir_deps += [fs_dep]
endif

cppgir = executable('cppgir',
  cppgir_sources,
  cpp_args : cppgir_args,
  dependencies : cppgir_deps,
  install : true,
  override_options : cppgir_overrides
)
meson.override_find_program('cppgir', cppgir)

# gi headers
expected_code = '''#include <expected>
auto f() -> std::expected<int, int> { return 2; }
'''
has_expected = compiler.compiles(expected_code, name : 'std::expected check')

pkgconfig = import('pkgconfig')

pkgconfig.generate(name: 'cppgir',
  version : meson.project_version(),
  subdirs : ['cppgir', 'cppgir' / 'gi', 'cppgir' / 'override'],
  description : 'GObject Introspection C++ wrapper generator.'
)

install_subdir('gi', install_dir : 'include' / 'cppgir')
install_subdir('override', install_dir : 'include' / 'cppgir')

inc = include_directories('.', 'gi', 'override')
if not has_expected
  expected_lite_include = 'expected-lite' / 'include'
  if not fs.exists(expected_lite_include / 'nonstd' / 'expected.hpp')
    error('missing submodule expected-lite')
  endif
  # Add an include option and copy the directory is all we have to do.
  # cppgir will automatically switch to `nonstd/expected.hpp` if it exists.
  inc = [inc] + include_directories(expected_lite_include)
  install_subdir(expected_lite_include / 'nonstd',
    install_dir : 'include' / 'cppgir' / 'gi')
endif

cppgir_dep = declare_dependency(include_directories: inc)
meson.override_dependency('cppgir', cppgir_dep)
