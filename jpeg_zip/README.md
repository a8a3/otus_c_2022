## prints out list of ZIPped files ##

### how to build: ###
1. meson setup build
2. cd build
3. meson compile
4. ninja -C . clang-format
5. ninja -C . clang-tidy
or
#### src/main.c -Wall -Wextra -Wpedantic -o jpeg_zip ####