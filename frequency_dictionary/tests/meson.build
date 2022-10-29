sources = [
    'src/dictionary_test.c'
]

# dependencies
cmocka = subproject('cmocka')
cmocka_dep = cmocka.get_variable('cmocka_dep')

executable(
    'dictionary_test', 
    sources,
    dependencies : cmocka_dep
)