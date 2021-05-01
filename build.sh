#!/bin/sh

OUT_FILE='myos.bin'

# compile
i686-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c printf.c -o printf.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c strings.c -o strings.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -c vga.c -o vga.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-as -o boot.o boot.s

# link
i686-elf-gcc -T linker.ld -o $OUT_FILE -ffreestanding -O2 -nostdlib \
    boot.o          \
    kernel.o        \
    printf.o        \
    strings.o       \
    vga.o           \
    -lgcc
