project(
    'words_count', ['c'],
    default_options : ['c_std=c11'],
    version: '0.0.1'
)

sources = ['src/main.c']
incdir = include_directories('../include')

executable(
    meson.project_name(), 
    sources,
    include_directories: incdir,
    c_args: ['-Wall', '-Wextra', '-Wpedantic']
)

subdir('tests')