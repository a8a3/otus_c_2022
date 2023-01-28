project(
    'http_parser', ['c'],
    default_options : ['c_std=c11'],
    version: '0.0.1'
)

sources = ['src/main.c']
include = include_directories('../frequency_dictionary/include')

executable(
    meson.project_name(), 
    sources,
    include_directories: include,
    c_args: ['-Wall', '-Wextra', '-Wpedantic']
)

