/* The bootloader will look at this image and start execution at the symbol
 * designated as the entry point. */
ENTRY(_start)

/* Tell where the various sections of the object files will be put in the final
 * kernel image. */
SECTIONS {
    /* Begin putting sections at 1 MiB, a conventional place for kernels to be
     * loaded at by the bootloader. '.' is the location counter and has an
     * initial value of 0. */
    . = 1M;

    /* First put the multiboot header, as it is required to be put very early
     * in the image or the bootloader won't recognize the file format. Right
     * after it we put the input '.text' (code) section. */
    /* The output '.text' section is aligned on 4k blocks. */
    .text : ALIGN(4K) {
        /* The '*' character is a wildcard which matches any file name. The
         * expression '*(.text)' means all '.text' input section in all input
         * files. */
        *(.multiboot)
        *(.text)
    }

    /* Read-only data. */
    .rodata : ALIGN(4K) {
        *(.data)
    }

    /* Read-write data (initialized). */
    .data : ALIGN(4K) {
        *(.data)
    }

    /* Read-write data (uninitialized) and stack. */
    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}
