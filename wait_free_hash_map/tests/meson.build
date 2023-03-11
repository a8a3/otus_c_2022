sources = ['wtf_table_ut.c']
incdir = include_directories('../include')

# dependencies
cmocka = subproject('cmocka')
cmocka_dep = cmocka.get_variable('cmocka_dep')

executable(
    'wtf_table_ut', 
    sources,
    include_directories: incdir,
    dependencies: cmocka_dep
)
