project(
    'weather', ['c'],
    default_options : ['c_std=c11'],
    version: '0.0.1'
)

sources = ['src/main.c']
curl_dep = dependency('libcurl', method: 'pkg-config')
glib_json_dep = dependency('json-glib-1.0', method: 'pkg-config')

executable(
    meson.project_name(), 
    sources,
    c_args: ['-Wall', '-Wextra', '-Wpedantic'],
    dependencies: [curl_dep, glib_json_dep]
)
