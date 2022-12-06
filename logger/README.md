## naive logging library

### features:
- macro definition based interface
- not thread safe
- file or console destination
- several instances are possible at the same time

### how to build:
```
1. meson setup _build -Ddefault_library=<static/shared/both> -Dbuildtype=<debug/release> -Db_sanitize=<none/address/thread/undefined/memory/leak>
2. cd _build
3. meson compile
```

### clang tools support:
```
ninja -C . clang-format
```