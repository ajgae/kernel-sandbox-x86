# Kernel sandbox

my experimentations with bare-metal x86 stuff

## testing the kernel

Run the `build.sh` script with an `i386-elf-gcc` compiler and `i386-elf` binutils suite in your path (IMPORTANT, will not work with tools that aren't targeting `i386-elf`).

Then, run `qemu-system-i386 -kernel myos.bin`.

You can also create an `iso` disk image using `grub-mkrescue` and `cp` that to a USB drive, then boot from that USB drive on your computer. In the current state of things, just following the instructions in [this section](https://wiki.osdev.org/Bare_Bones#Building_a_bootable_cdrom_image) should be enough to get a working bootable image. Depending on the computer you're booting it from, you might need to tweak the grub command line settings to give an appropriate console to the OS. For example, on my Thinkpad X1, with the exact config described on the OSdev.org page, grub gives an error, but it boots fine out-of-the-box on older thinkpads (e.g. X200s or X61).
