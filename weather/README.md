# weather
shows current weather in the specified city

### features
- uses libcurl to grab data in JSON format from wttr.in via https request
- uses JSON-GLib to parse grabbed JSON

### dependencies
libcurl and JSON-GLib libraries must be available in your environment

### how to build:
1. `meson setup _build .`
2. `meson compile -C _build`
3. `ninja -C _build clang-format`
4. `ninja -C _build clang-tidy`

### how to run:
`weather [-c city]`