incdir = include_directories('../include')
thread_dep = dependency('threads')

executable(
    'wtf_table_sample',
    'wtf_table_sample.c',
    include_directories: incdir,
    c_args: ['-Wall', '-Wextra', '-Wpedantic', '-O1'],
    dependencies: thread_dep,
#   link_args: ['-ltcmalloc']
)

executable(
    'tools_test',
    'tools_test.c',
    c_args: ['-Wall', '-Wextra', '-Wpedantic', '-O1'],
    dependencies: thread_dep,
#   link_args: ['-ltcmalloc']
)
