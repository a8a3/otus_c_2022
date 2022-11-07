sources = [
    'src/dictionary_test.c'
]

incdir = include_directories('../include')

# dependencies
cmocka = subproject('cmocka')
cmocka_dep = cmocka.get_variable('cmocka_dep')

executable(
    'dictionary_test', 
    sources,
    include_directories: incdir,
    dependencies: cmocka_dep
)