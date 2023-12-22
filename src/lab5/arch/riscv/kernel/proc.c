//arch/riscv/kernel/proc.c
#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"
#include "test.h"
#include "elf.h"
#include "string.h"

//arch/riscv/kernel/proc.c

extern void __dummy();

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

/**
 * new content for unit test of 2023 OS lab2
*/
extern uint64 task_test_priority[]; // test_init 后, 用于初始化 task[i].priority 的数组
extern uint64 task_test_counter[];  // test_init 后, 用于初始化 task[i].counter  的数组
extern create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm);
extern unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
extern char _sramdisk[], _eramdisk[];
static uint64_t load_program(struct task_struct* task, pagetable_t pgtbl);
void task_init() {
    test_init(NR_TASKS);
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle

    /* YOUR CODE HERE */
    idle = (struct task_struct *)kalloc();
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;
    current = idle;
    task[0] = idle;

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, 此外，为了单元测试的需要，counter 和 priority 进行如下赋值：
    //      task[i].counter  = task_test_counter[i];
    //      task[i].priority = task_test_priority[i];
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址

    /* YOUR CODE HERE */
    for(int i=1; i<NR_TASKS; ++i) {
        task[i] = (struct task_struct *)kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->counter = task_test_counter[i];
        task[i]->priority = task_test_priority[i];
        task[i]->pid = i;
        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = (uint64)task[i] + PGSIZE;
        // task[i]->thread.sepc = USER_START;
        // uint64 sstatus = csr_read(sstatus);
        // sstatus &= ~(1 << 8);       // SPP = 0
        // sstatus |= 1 << 5;          // SPIE = 1
        // sstatus |= 1 << 18;         // SUM = 1
        // task[i]->thread.sstatus = sstatus;
        // task[i]->thread.sscratch = USER_END;
        task[i]->pgd = (pagetable_t)alloc_page();        // 创建属于每个进程的页表
        for(int j=0; j<512; ++j) {          // 将内核页表复制到每个进程的页表中
            task[i]->pgd[j] = swapper_pg_dir[j];
        }
        // uint64_t num;
        // if(((uint64)_eramdisk - (uint64)_sramdisk) % PGSIZE == 0) num = ((uint64)_eramdisk - (uint64)_sramdisk) / PGSIZE;
        // else num = ((uint64)_eramdisk - (uint64)_sramdisk) / PGSIZE + 1;
        // char *_uapp = (char *)alloc_pages(num);
        // for(int j=0; j<(uint64)_eramdisk - (uint64)_sramdisk; ++j) {    // 二进制文件需要先被拷贝到一块某个进程专用的内存之后再进行映射
        //     _uapp[j] = _sramdisk[j];
        // }
        // create_mapping(task[i]->pgd, USER_START, (uint64)_uapp-PA2VA_OFFSET, num * PGSIZE, 0x1F);   // 将 _uapp 所在的页面映射到每个进行的页表中
        // task[i]->thread_info.kernel_sp = (uint64)task[i] + PGSIZE;      // 设置内核态栈
        // task[i]->thread_info.user_sp = (uint64)alloc_page();            // 设置用户态栈
        // create_mapping(task[i]->pgd, USER_END-PGSIZE, task[i]->thread_info.user_sp-PA2VA_OFFSET, PGSIZE, 0x17);      // 映射到进程页表中（无执行权限）
        load_program(task[i], task[i]->pgd);
    }

    printk("...proc_init done!\n");
}

static uint64_t load_program(struct task_struct* task, pagetable_t pgtbl) {
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)_sramdisk;

    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff;
    int phdr_cnt = ehdr->e_phnum;

    Elf64_Phdr* phdr;
    int load_phdr_cnt = 0;
    for (int i = 0; i < phdr_cnt; i++) {
        phdr = (Elf64_Phdr*)(phdr_start + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD) {
            // alloc space and copy content
            // do mapping
            // char *_start = (char *)(_sramdisk + phdr->p_offset);        // uapp在elf中的起始位置
            // uint64 num;         // 需要连续分配多少页
            // if(phdr->p_memsz % PGSIZE == 0) num = phdr->p_memsz / PGSIZE;
            // else num = phdr->p_memsz / PGSIZE + 1;
            // char *_uapp = (char *)alloc_pages(num);
            // for(int j=0; j<phdr->p_memsz; ++j) {            // 拷贝
            //     _uapp[j] = _start[j];
            // }
            // memset((char *)((uint64)_uapp + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);        // 将物理区间清零

            // uint64 va = phdr->p_vaddr;
            // uint64 offset = va % PGSIZE;            // va相对于整数倍PGSIZE的页数
            // if(va + phdr->p_memsz + offset > num * PGSIZE) {    // 由于需要右移，原来分配的页数可能不够
            //     num++;
            //     free_pages((uint64)_uapp);
            //     _uapp = (char *)alloc_pages(num);
            // }
            // for(int j = num * PGSIZE - 1; j >= offset; --j) {      // 整体右移offset，因为映射需要的是页的基地址
            //     _uapp[j] = _uapp[j-offset];
            // }
            // memset(_uapp, 0, offset);           // 第一页开头部分清零
            // create_mapping(pgtbl, va-offset, (uint64)_uapp-PA2VA_OFFSET, num * PGSIZE, 0x1F);
            uint64 flags = phdr->p_flags << 1;      // XWR权限 + ANONYM bit（为0表示非匿名）
            do_mmap(task, phdr->p_vaddr, phdr->p_memsz, flags, phdr->p_offset, phdr->p_filesz);
        }
    }
    // set user stack
    // task->thread_info.user_sp = (uint64)alloc_page();
    // allocate user stack and do mapping
    // create_mapping(pgtbl, USER_END-PGSIZE, task->thread_info.user_sp-PA2VA_OFFSET, PGSIZE, 0x17);
    do_mmap(task, USER_END-PGSIZE, PGSIZE, VM_R_MASK | VM_W_MASK | VM_ANONYM, 0, 0);     // 用户栈是匿名的区域
    // pc for the user program
    task->thread.sepc = ehdr->e_entry;
    // sstatus bits set
    uint64 sstatus = csr_read(sstatus);
    sstatus &= ~(1 << 8);       // SPP = 0
    sstatus |= 1 << 5;          // SPIE = 1
    sstatus |= 1 << 18;         // SUM = 1
    task->thread.sstatus = sstatus;
    // user stack for user program
    task->thread.sscratch = USER_END;
}

// arch/riscv/kernel/proc.c
void dummy() {
    schedule_test();
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while(1) {
        if ((last_counter == -1 || current->counter != last_counter) && current->counter > 0) {
            if(current->counter == 1){
                --(current->counter);   // forced the counter to be zero if this thread is going to be scheduled
            }                           // in case that the new counter is also 1, leading the information not printed.
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
        }
    }
}

// arch/riscv/kernel/proc.c

extern void __switch_to(struct task_struct* prev, struct task_struct* next);

void switch_to(struct task_struct* next) {
    /* YOUR CODE HERE */
    if(next != current) {
        struct task_struct* prev = current;
        current = next;
        __switch_to(prev, next);
    }
}

// arch/riscv/kernel/proc.c

void do_timer(void) {
    // 1. 如果当前线程是 idle 线程 直接进行调度
    // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度

    /* YOUR CODE HERE */
    if(current == idle || current->counter == 0) schedule();
    else {
        if(--current->counter > 0)  return;
        else schedule();
    }
}

// arch/riscv/kernel/proc.c
#ifdef SJF
void schedule(void) {
    /* YOUR CODE HERE */
    uint64 c = 0, next;
    for(int i=1; i<NR_TASKS; ++i) {
        uint64 cnt = task[i]->counter;
        if(task[i]->state != TASK_RUNNING || cnt == 0) continue;
        if(c == 0 || cnt < c) {
            c = cnt;
            next = i;
        }
    }
    if(c) {
        printk("switch to [PID = %d COUNTER = %d]\n", task[next]->pid, task[next]->counter);
        switch_to(task[next]);
    }
    else {
        for(int i=0; i<NR_TASKS; ++i) {
            task[i]->counter = rand();
        }
        schedule();
    }
}
#endif

// arch/riscv/kernel/proc.c
#ifdef PRIORITY
void schedule(void) {
    /* YOUR CODE HERE */
    uint64 c, next, i;
    while(1) {
        c = 0;
        next = 0;
        i = NR_TASKS;
        while(--i) {
            if(!task[i]) continue;
            uint64 cnt = task[i]->counter;
            if(task[i]->state != TASK_RUNNING || cnt == 0) continue;
            if(cnt > c) {
                c = cnt;
                next = i;
            }
        }
        if(c) break;
        for(i=NR_TASKS-1; i>0; --i) {
            task[i]->counter = (task[i]->counter >> 1) + task[i]->priority;
        }
    }
    printk("switch to [PID = %d COUNTER = %d PRIORITY = %d]\n", task[next]->pid, task[next]->counter, task[next]->priority);
    switch_to(task[next]);
}
#endif

void do_mmap(struct task_struct *task, uint64_t addr, uint64_t length, uint64_t flags,
    uint64_t vm_content_offset_in_file, uint64_t vm_content_size_in_file) {
    struct vm_area_struct *vma = &(task->vmas[task->vma_cnt]);
    vma->vm_start = addr;
    vma->vm_end = addr + length;
    vma->vm_flags = flags;
    vma->vm_content_offset_in_file = vm_content_offset_in_file;
    vma->vm_content_size_in_file = vm_content_size_in_file;
    task->vma_cnt++;
}

struct vm_area_struct *find_vma(struct task_struct *task, uint64_t addr) {
    for(uint64_t i=0; i<task->vma_cnt; i++) {
        struct vm_area_struct *vma = &(task->vmas[i]);
        if(vma->vm_start <= addr && addr <= vma->vm_end) {
            return vma;
        }
    }
    return NULL;
}