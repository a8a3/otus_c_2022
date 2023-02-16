# telehack_clt
telehack.com client. transforms specified text via `figlet` command

### how to build:
1. `meson setup build <-Db_sanitize=address | memory | undefined | leak>`
2. `meson compile -C build`
3. `ninja -C build clang-format`

### how to run:
`telehack_clt -f <font> -t <text>`

