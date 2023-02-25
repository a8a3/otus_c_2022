# http_srv 
Shares files via HTTP protocol.
Waits for clients connection on the specified adress.

### how to build:
1. `meson setup build`
2. `meson compile -C build`
3. `ninja -C build clang-format`

### how to run:
`http_srv -d <dir_path> -a <ip address>:<port>`

