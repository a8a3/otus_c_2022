project('logger', 'c', 
    version: '0.0.1',
    default_options : ['b_ndebug=if-release']
)

lib_name = meson.project_name()

if get_option('buildtype') == 'debug'
    lib_name = lib_name + 'd'
endif

inc_dir = include_directories('include')

logger_lib = library(
    lib_name, 
    'src/logger.c',
    include_directories: inc_dir,
    c_args: ['-std=c11', '-Wall', '-Wextra', '-Wpedantic'],
    version: meson.project_version(),
    soversion: '0'
)

subdir('sample')
