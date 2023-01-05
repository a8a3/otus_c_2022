project(
    'fszd', ['c'],
    default_options : ['c_std=c11'],
    version: '0.0.1'
)

sources = ['src/main.c']

executable(
    meson.project_name(), 
    sources,
    c_args: ['-Wall', '-Wextra', '-Wpedantic']
)
