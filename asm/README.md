## asm sandbox

### content:
- [original assembly](asm/main.asm)
- [original assembly + some comments on what is going on](asm/main.info.asm)
- [assembly with stack aligning](asm/main.deb.asm)
- [assembly with stack aligning, fixed memory leaks + optimized m/f procs](asm/main.deb.fixed.asm)


- [C implementation](c/main.c)


### how to build:
```
cd asm && make
cd c && make
```

### how to debug:
```
ddd <executable>
```
