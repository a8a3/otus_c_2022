## frequency dictionary ##

### features
- uses cmocka C testing framework

### how to build:
1. `meson setup build`
2. `cd build`
3. `meson compile`
4. `ninja -C . clang-format`
5. `ninja -C . clang-tidy`

### or
`clang src/main.c -Iinclude -Wall -Wextra -Wpedantic -Werror -o frequency_dict`

### how to run:
`frequency_dict [-f input_file]`