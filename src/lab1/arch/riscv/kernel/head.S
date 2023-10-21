.extern start_kernel

    .section .text.init
    .globl _start
_start:
    # set stvec = _traps
    la t0, _traps
    csrw stvec, t0

    # set sie[STIE] = 1
    csrr t0, sie
    ori t0, t0, 32
    csrw sie, t0

    # set first time interrupt
    mv a7, x0
    mv a6, x0
    rdtime t0
    la t1, 10000000
    add a0, t0, t1
    mv a1, x0
    mv a2, x0
    mv a3, x0
    mv a4, x0
    mv a5, x0
    ecall

    # set sstatus[SIE] = 1
    csrr t0, sstatus
    ori t0, t0, 2
    csrw sstatus, t0

    la sp, boot_stack_top
    call start_kernel


    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top: