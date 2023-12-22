// trap.c 
#include "syscall.h"
#include "proc.h"
#include "defs.h"
#include <string.h>

extern struct task_struct* current;
extern char _sramdisk[];

void do_page_fault(struct pt_regs *regs) {
    /*
     1. 通过 stval 获得访问出错的虚拟内存地址（Bad Address）
     2. 通过 find_vma() 查找 Bad Address 是否在某个 vma 中
     3. 分配一个页，将这个页映射到对应的用户地址空间
     4. 通过 (vma->vm_flags & VM_ANONYM) 获得当前的 VMA 是否是匿名空间
     5. 根据 VMA 匿名与否决定将新的页清零或是拷贝 uapp 中的内容
    */
    uint64 bad_addr = regs->stval;
    struct vm_area_struct *vma = find_vma(current, bad_addr);
    if(vma != NULL) {
        char *alloc = (char *)alloc_page();
        uint64 perm = vma->vm_flags | 0x11;     // vm_flags原有XWR权限 + U bit + V bit
        create_mapping(current->pgd, PGROUNDDOWN(bad_addr), (uint64)alloc-PA2VA_OFFSET, PGSIZE, perm);      // 映射到对应的用户地址空间
        memset(alloc, 0, PGSIZE);               // 清空
        if(!(vma->vm_flags & VM_ANONYM)) {      // 不是匿名空间
            // 将 segment 的内容从ELF文件中读入到[p_vaddr, p_vaddr + p_memsz) 这一连续区间
            char *src = _sramdisk + vma->vm_content_offset_in_file;
            if(bad_addr - vma->vm_start < PGSIZE) {         // 如果bad_addr跟vma的开头可以放在同一页内
                uint64 offset = vma->vm_start % PGSIZE;     // 计算开头偏移量
                if(vma->vm_end - offset < PGSIZE)           // 整个uapp可以放进一个页里
                    for(uint64 i=0; i<vma->vm_end - vma->vm_start; ++i)
                        alloc[offset+i] = src[i];
                else        // 放不下，则应该将[offset, PGSIZE]的部分拷贝进去
                    for(uint64 i=0; i<PGSIZE - offset; ++i)
                        alloc[offset+i] = src[i];
            }
            else {          // vma开头在前面的页中，表明该页一开始便应该拷贝uapp的内容
                if(vma->vm_end - bad_addr < PGSIZE)         // 如果bad_addr跟vma的末尾可以放在同一页内
                    for(uint64 i=0; i<vma->vm_end % PGSIZE; ++i)    // 将[0, vm_end%PGSIZE]拷贝进去
                        alloc[i] = src[i];
                else        // 放不下，则拷贝[0, PGSIZE]内容
                    for(uint64 i=0; i<PGSIZE; ++i)
                        alloc[i] = src[i];
            }
            // 将 [p_vaddr + p_filesz, p_vaddr + p_memsz) 对应的物理区间清零
            uint64 file_end = vma->vm_start + vma->vm_content_size_in_file;     // p_vaddr + p_filesz
            if(PGROUNDDOWN(bad_addr) <= file_end && file_end <= PGROUNDUP(bad_addr)) {      // 如果file_end就在这一页内
                uint64 sz = PGROUNDUP(bad_addr) - file_end;
                memset(alloc+PGSIZE-sz, 0, sz);     // 将file_end之后的物理区域清零
            }
            else if(file_end < PGROUNDDOWN(bad_addr)) {         // 如果file_end在之前的页里
                memset(alloc, 0, PGSIZE);       // 将整个页都清空
            }
        }
    }
}

void trap_handler(uint64_t scause, uint64_t sepc, struct pt_regs *regs) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.3.4 节
    // 其他interrupt / exception 可以直接忽略
    if (scause >> 63) {         // interrupt
        if (scause & 5) {       // timer interrupt
            printk("[S] Supervisor Mode Timer Interrupt\n");
            clock_set_next_event();
            do_timer();
        }
    }
    else if(scause == 8) {
        uint64_t sys_call_num = regs->x[17];
        if(sys_call_num == 64) {
            regs->x[10] = sys_write(regs->x[10], regs->x[11], regs->x[12]);
            regs->sepc += 4;
        }
        else if(sys_call_num == 172) {
            regs->x[10] = sys_getpid();
            regs->sepc += 4;
        }
        else if(sys_call_num == 220) {

        }
        else {
            printk("[S] Unhandled syscall: %lx", sys_call_num);
            while (1);
        }
    }
    else if(scause == 12 || scause == 13 || scause == 15) {
        printk("[S] Page Fault, ");
        printk("scause: %lx, ", scause);
        printk("stval: %lx, ", regs->stval);
        printk("sepc: %lx\n", regs->sepc);
        do_page_fault(regs);
    }
    else {
        printk("[S] Unhandled trap, ");
        printk("scause: %lx, ", scause);
        printk("stval: %lx, ", regs->stval);
        printk("sepc: %lx\n", regs->sepc);
        while (1);
    }
}