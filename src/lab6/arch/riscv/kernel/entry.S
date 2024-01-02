    .section .text.entry
    .align 2
    .globl _traps 
_traps:
    csrr t0, sscratch
    beq t0, x0, _old_traps  # in S mode
    csrw sscratch, sp
    mv sp, t0
_old_traps:
    # 1. save 32 registers and sepc to stack
    addi sp, sp, -37*8
    sd x0, 0*8(sp)
    sd x1, 1*8(sp)
    sd x2, 2*8(sp)
    sd x3, 3*8(sp)
    sd x4, 4*8(sp)
    sd x5, 5*8(sp)
    sd x6, 6*8(sp)
    sd x7, 7*8(sp)
    sd x8, 8*8(sp)
    sd x9, 9*8(sp)
    sd x10, 10*8(sp)
    sd x11, 11*8(sp)
    sd x12, 12*8(sp)
    sd x13, 13*8(sp)
    sd x14, 14*8(sp)
    sd x15, 15*8(sp)
    sd x16, 16*8(sp)
    sd x17, 17*8(sp)
    sd x18, 18*8(sp)
    sd x19, 19*8(sp)
    sd x20, 20*8(sp)
    sd x21, 21*8(sp)
    sd x22, 22*8(sp)
    sd x23, 23*8(sp)
    sd x24, 24*8(sp)
    sd x25, 25*8(sp)
    sd x26, 26*8(sp)
    sd x27, 27*8(sp)
    sd x28, 28*8(sp)
    sd x29, 29*8(sp)
    sd x30, 30*8(sp)
    sd x31, 31*8(sp)
    csrr t0, sepc
    sd t0, 32*8(sp)
    csrr t0, sstatus
    sd t0, 33*8(sp)
    csrr t0, stval
    sd t0, 34*8(sp)
    csrr t0, sscratch
    sd t0, 35*8(sp)
    csrr t0, scause
    sd t0, 36*8(sp)

    # 2. call trap_handler
    csrr a0, scause
    csrr a1, sepc
    mv a2, sp
    call trap_handler

    .global __ret_from_fork
__ret_from_fork:
    # 利用 sp 从栈中恢复出寄存器的值
    # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
    ld t0, 36*8(sp)
    csrw scause, t0
    ld t0, 35*8(sp)
    csrw sscratch, t0
    ld t0, 34*8(sp)
    csrw stval, t0
    ld t0, 33*8(sp)
    csrw sstatus, t0
    ld t0, 32*8(sp)
    csrw sepc, t0
    ld x0, 0*8(sp)
    ld x1, 1*8(sp)
    ld x3, 3*8(sp)
    ld x4, 4*8(sp)
    ld x5, 5*8(sp)
    ld x6, 6*8(sp)
    ld x7, 7*8(sp)
    ld x8, 8*8(sp)
    ld x9, 9*8(sp)
    ld x10, 10*8(sp)
    ld x11, 11*8(sp)
    ld x12, 12*8(sp)
    ld x13, 13*8(sp)
    ld x14, 14*8(sp)
    ld x15, 15*8(sp)
    ld x16, 16*8(sp)
    ld x17, 17*8(sp)
    ld x18, 18*8(sp)
    ld x19, 19*8(sp)
    ld x20, 20*8(sp)
    ld x21, 21*8(sp)
    ld x22, 22*8(sp)
    ld x23, 23*8(sp)
    ld x24, 24*8(sp)
    ld x25, 25*8(sp)
    ld x26, 26*8(sp)
    ld x27, 27*8(sp)
    ld x28, 28*8(sp)
    ld x29, 29*8(sp)
    ld x30, 30*8(sp)
    ld x31, 31*8(sp)
    ld x2, 2*8(sp)      # 最后读取x2(sp)
    addi sp, sp, 37*8

    csrr t0, sscratch
    beq t0, x0, _sret
    csrw sscratch, sp
    mv sp, t0
_sret:
    # 4. return from trap
    sret

# arch/riscv/kernel/entry.S

    .global __dummy
__dummy:
    # YOUR CODE HERE
    csrr t0, sscratch
    csrw sscratch, sp
    mv sp, t0
    sret

# arch/riscv/kernel/entry.S

    .globl __switch_to
__switch_to:
    # save state to prev process
    # YOUR CODE HERE
    sd ra, 48(a0)       # ra是task_struct上第7个uint64
    sd sp, 56(a0)
    sd s0, 64(a0)
    sd s1, 72(a0)
    sd s2, 80(a0)
    sd s3, 88(a0)
    sd s4, 96(a0)
    sd s5, 104(a0)
    sd s6, 112(a0)
    sd s7, 120(a0)
    sd s8, 128(a0)
    sd s9, 136(a0)
    sd s10, 144(a0)
    sd s11, 152(a0)
    csrr t1, sepc
    sd t1, 160(a0)
    csrr t1, sstatus
    sd t1, 168(a0)
    csrr t1, sscratch
    sd t1, 176(a0)

    # restore state from next process
    # YOUR CODE HERE
    ld ra, 48(a1)
    ld sp, 56(a1)
    ld s0, 64(a1)
    ld s1, 72(a1)
    ld s2, 80(a1)
    ld s3, 88(a1)
    ld s4, 96(a1)
    ld s5, 104(a1)
    ld s6, 112(a1)
    ld s7, 120(a1)
    ld s8, 128(a1)
    ld s9, 136(a1)
    ld s10, 144(a1)
    ld s11, 152(a1)
    ld t1, 160(a1)
    csrw sepc, t1
    ld t1, 168(a1)
    csrw sstatus, t1
    ld t1, 176(a1)
    csrw sscratch, t1

    # 计算satp
    ld t1, 184(a1)      # pgd
    la t2, PA2VA_OFFSET
    sub t1, t1, t2      # 还原至物理地址
    srli t1, t1, 12     # PPN
    addi t2, x0, 8
    slli t2, t2, 60     # MODE
    or t1, t1, t2
    csrw satp, t1       # 写入satp

    # 刷新 TLB 和 ICache
    sfence.vma zero, zero
    fence.i

    ret