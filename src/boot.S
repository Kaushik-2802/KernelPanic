/* Constants for the Multiboot Header */
.set ALIGN,    1<<0             
.set MEMINFO,  1<<1            
.set FLAGS,    ALIGN | MEMINFO  

.set MAGIC,    0x1BADB002       
.set CHECKSUM, -(MAGIC + FLAGS) 

/* The Multiboot Header */
.section .multiboot_header
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:


.section .text
.global start
.type start, @function
start:
    mov $stack_top, %esp
    cli
    call kernel_main
    cli
1:  hlt
    jmp 1b
