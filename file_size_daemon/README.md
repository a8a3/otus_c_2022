# fszd. File SiZe Daemon.
shows size of specified file in bytes

### how to build:
1. `meson setup _build .`
2. `meson compile -C _build`
3. `ninja -C _build clang-format`
4. `ninja -C _build clang-tidy`

### how to run:
`fszd [-d daemon-mode]`
`fszd <file name>`

### print help
`fszd -h`
