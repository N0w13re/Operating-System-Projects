.extern start_kernel

    .section .text.init
    .globl _start
_start:
    la sp, boot_stack_top   # 此处是虚拟地址
    la t0, PA2VA_OFFSET
    sub sp, sp, t0      # 还原至物理地址
    call setup_vm
    call relocate
    call mm_init
    call setup_vm_final
    call task_init
    
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
    # csrr t0, sstatus
    # ori t0, t0, 2
    # csrw sstatus, t0

    call start_kernel

relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)

    ###################### 
    #   YOUR CODE HERE   #
    ######################
    la t0, PA2VA_OFFSET
    add ra, ra, t0
    add sp, sp, t0

    # set satp with early_pgtbl

    ###################### 
    #   YOUR CODE HERE   #
    ######################
    la t1, early_pgtbl
    sub t1, t1, t0      # 还原至物理地址
    srli t1, t1, 12     # PPN
    addi t2, x0, 8
    slli t2, t2, 60     # MODE
    or t1, t1, t2
    csrw satp, t1       # 写入satp

    # flush tlb
    sfence.vma zero, zero

    # flush icache
    fence.i

    ret

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top: