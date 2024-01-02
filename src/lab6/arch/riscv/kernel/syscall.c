#include "syscall.h"
#include "proc.h"
#include "defs.h"
#include <string.h>

extern struct task_struct* current;        // 指向当前运行线程的 `task_struct`
extern struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此
extern create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm);
extern void __ret_from_fork();
extern unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
extern char _sramdisk[], _eramdisk[];

long sys_write(unsigned int fd, const char* buf, size_t count) {
    long ret = 0;
    for(int i=0; i<count; ++i) {
        if(fd == 1) {
            printk("%c", buf[i]);
            ++ret;
        }
    }
    return ret;
}

long sys_getpid() {
    return current->pid;
}

uint64_t sys_clone(struct pt_regs *regs) {
    /*
     1. 参考 task_init 创建一个新的 task，将的 parent task F 
        task_struct 页上(这一步复制了哪些东西?）。将 thread.ra 设置为 
        __ret_from_fork，并正确设置 thread.sp
        (仔细想想，这个应该设置成什么值?可以根据 child task 的返回路径来倒推)

     2. 利用参数 regs 来计算出 child task 的对应的 pt_regs 的地址，
        并将其中的 a0, sp, sepc 设置成正确的值(为什么还要设置 sp?)

     3. 为 child task 申请 user stack，并将 parent task 的 user stack 
        数据复制到其中。 (既然 user stack 也在 vma 中，这一步也可以直接在 5 中做，无需特殊处理)

     3.1. 同时将子 task 的 user stack 的地址保存在 thread_info->
        user_sp 中，如果你已经去掉了 thread_info，那么无需执行这一步

     4. 为 child task 分配一个根页表，并仿照 setup_vm_final 来创建内核空间的映射

     5. 根据 parent task 的页表和 vma 来分配并拷贝 child task 在用户态会用到的内存

     6. 返回子 task 的 pid
    */

    // 1.
    struct task_struct *child = (struct task_struct *)kalloc();;
    memcpy((void *)child, (void *)current, PGSIZE);
    for(int i=2; i<NR_TASKS; ++i) {      // 寻找空闲pid
        if(task[i] == NULL) {
            child->pid = i;
            task[i] = child;
            break;
        }
        if(i == NR_TASKS - 1) {
            printk("No free pid!\n");
            return -1;
        }
    }

    // 2.
    child->thread.ra = (uint64)&__ret_from_fork;
    struct pt_regs *child_regs = (uint64)child + (uint64)regs - (uint64)current;      // child_regs与child的距离 == regs与current的距离
    child->thread.sp = (uint64)child_regs;
    
    child_regs->x[10] = 0;      // 返回值为0
    child_regs->x[2] = (uint64)child_regs;      // sp为child_regs地址
    child_regs->sepc += 4;

    // 3.
    uint64 child_stack = (uint64)alloc_page();      // 为 child task 申请 user stack
    memcpy(child_stack, PGROUNDDOWN(regs->sscratch), PGSIZE);       // regs->sscratch存储的是parent用户态的sp
    child->thread_info.user_sp = child_stack;       // useless

    // 4.
    child->pgd = (pagetable_t)alloc_page();         // 为 child task 分配一个根页表
    memcpy(child->pgd, swapper_pg_dir, PGSIZE);     // 拷贝内核态页表
    create_mapping(child->pgd, USER_END - PGSIZE, child_stack - PA2VA_OFFSET, PGSIZE, 0x17);    // child用户栈，权限为U|W|R|V

    // 5.
    for(int i=0; i<current->vma_cnt; ++i) {
        struct vm_area_struct *vma = &(current->vmas[i]);
        for(uint64 va = PGROUNDDOWN(vma->vm_start); va < vma->vm_end; va += PGSIZE) {
            pagetable_t pgtbl = (pagetable_t)current->pgd;      // walk page table
            // 一级页表
            uint64 index = (va >> 30) & 0x1ff;     // VPN[2]
            if((pgtbl[index] & 1) == 0)     // V bit == 0, 不存在
                continue;
            uint64 *tbl = (uint64 *)(((pgtbl[index] >> 10) << 12) + PA2VA_OFFSET);      // 下一级页表的地址
            // 二级页表
            index = (va >> 21) & 0x1ff;     // VPN[1]
            if((tbl[index] & 1) == 0)
                continue;
            uint64 *tbl2 = (uint64 *)(((tbl[index] >> 10) << 12) + PA2VA_OFFSET);
            // 三级页表
            index = (va >> 12) & 0x1ff;     // VPN[2]
            if((tbl2[index] & 1) == 0)
                continue;
            uint64 alloc = alloc_page();
            create_mapping(child->pgd, va, alloc - PA2VA_OFFSET, PGSIZE, vma->vm_flags | 0x11);     // vm_flags原有XWR权限 + U bit + V bit
            memcpy((char *)alloc, (char *)va, PGSIZE);          // 拷贝那些已经分配并映射的页
        }
    }

    // 6.
    printk("[S] New task: %d\n", child->pid);
    return child->pid;
}