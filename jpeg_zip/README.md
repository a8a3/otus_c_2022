## prints out list of ZIPped files ##

<p><a href="https://en.wikipedia.org/wiki/ZIP_(file_format)" title="Pandoc-style short links">ZIP format description</a></p>

### how to build: ###
1. meson setup build
2. cd build
3. meson compile
4. ninja -C . clang-format
5. ninja -C . clang-tidy

### or ###
clang src/main.c -Wall -Wextra -Wpedantic -o jpeg_zip