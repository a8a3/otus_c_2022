AS = yasm 

all: asm-prog

asm-prog: main.deb.fixed.o
	$(CC) -g -no-pie $^ -o $@

main.deb.fixed.o: main.deb.fixed.asm
	$(AS) -g dwarf2 -f elf64 $^ -l main.deb.fixed.lst 

clean:
	rm -f asm-prog *.o *.lst core

PHONY: clean
