CC = clang
SS = nasm
LD = ld

out:
	$(SS) -felf64 src/main.s -o build/Elf.o
	$(LD) build/Elf.o -o Elf