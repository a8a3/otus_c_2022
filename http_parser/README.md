# http_parser 
parses http server logs in the specified directory, in several threads (1 file per thread)

### how to build:
1. `meson setup build`
2. `meson compile -C build`
3. `ninja -C build clang-format`

### how to run:
`http_parser -d <dir_path> -t <num_threads>`

### valgrind checks:
#### memory issues:
`valgrind --leak-check=yes --track-origins=yes ./http_parser -d <dir_path> -t <num_threads>`

#### concurrency issues:
`valgrind --tool=helgrind ./http_parser -d <dir_path> -t <num_threads>`
