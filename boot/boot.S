/* Multiboot 1 Header for i386 QEMU (-kernel) and GRUB ISO */
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384 /* 16 KiB stack */
stack_top:

.section .text
.global _start_multiboot
.type _start_multiboot, @function
_start_multiboot:
    mov $stack_top, %esp
    call kernel_main
    cli
1:  hlt
    jmp 1b
