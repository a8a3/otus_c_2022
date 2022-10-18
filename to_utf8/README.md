## cp-1251, koi8-r, iso-8859-5 to utf8 converter ##

### how to build:
1. meson setup build
2. cd build
3. meson compile
4. ninja -C . clang-format
5. ninja -C . clang-tidy

#### or
clang src/main.c -std=c11 -Wall -Wextra -Wpedantic -o to_utf

### how to use:
to_utf [-i <u>input_file</u>] [-e <u>input_file_encoding</u>] [-o <u>output_file_in_utf8</u>]

##### incoming file encoding is one of
- <p><a href="https://en.wikipedia.org/wiki/Windows-1251" title="Pandoc-style short links">cp-1251</a></p> 
- <p><a href="https://en.wikipedia.org/wiki/KOI8-R" title="Pandoc-style short links">koi8-r</a></p> 
- <p><a href="https://en.wikipedia.org/wiki/ISO/IEC_8859-5" title="Pandoc-style short links">iso-8859-5</a></p>