sources = ['wtf_table_ut.c']
incdir = include_directories('../include')

# dependencies
cmocka = subproject('cmocka')
cmocka_dep = cmocka.get_variable('cmocka_dep')

executable(
    'wtf_table_ut', 
    sources,
    c_args: ['-Wall', '-Wextra', '-Wpedantic'],
    include_directories: incdir,
    dependencies: cmocka_dep
)


#message('run valgrind checks...')
#run_command('valgrind', cfg_file, build_dir, capture: false, check: true)
