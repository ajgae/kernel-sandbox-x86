# Kernel sandbox

my experimentations with bare-metal x86 stuff

## testing the kernel

Run the `build.sh` script with an `i386-elf-gcc` compiler and `i386-elf` binutils suite in your path (IMPORTANT, will not work with tools that aren't targeting `i386-elf`).

Then, run `qemu-system-i386 -kernel myos.bin`.

For now I have only tested this kernel in qemu, but I am planning to test it on my machine some time in the future.
