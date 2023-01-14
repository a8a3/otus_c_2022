# fszd. File SiZe Daemon
shows the size of the file specified in configuration

### features:
- daemon mode/regular app mode
- YAML cfg file
- awaits incoming requests on `fszd.socket` UNIX domain socket
- rereads configuration on SIGHUP signal
- stops on SIGINT signal

### how to build:
1. `meson setup build .`
2. `meson compile -C build`
3. `ninja -C build clang-format`

### how to run:
`fszd -c [config_file_path] [-N no-daemon-mode]`

### how to test:
file size request:
`
nc -U fszd.socket
`
reread configuration request:
`kill -SIGHUP <pid>`

daemon stop request:
`kill -SIGINT <pid>`
