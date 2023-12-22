// arch/riscv/kernel/vm.c
#include "defs.h"
#include "string.h"
#include "mm.h"
#include "printk.h"

/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long  early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    memset(early_pgtbl, 0x0, PGSIZE);
    unsigned long va, pa, index, entry;
    va = 0x80000000;
    pa = va;     // 等值映射
    index = (va >> 30) & 0x1ff;     // 中间9 bit作为index
    entry = (pa >> 12) << 10;       // PPN
    entry |= 0xf;                   // V | R | W | X 位设置为 1
    early_pgtbl[index] = entry;
    
    va = 0xffffffe000000000;        // 将其映射到 direct mapping area 
    index = (va >> 30) & 0x1ff;
    early_pgtbl[index] = entry;
}

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
extern char _stext[], _srodata[], _sdata[];

void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    uint64 va = VM_START + OPENSBI_SIZE, pa = PHY_START + OPENSBI_SIZE, sz = _srodata - _stext;
    create_mapping(swapper_pg_dir, va, pa, sz, 11);
    
    // mapping kernel rodata -|-|R|V
    va += sz;
    pa += sz;
    sz = _sdata - _srodata;
    create_mapping(swapper_pg_dir, va, pa, sz, 3);

    // mapping other memory -|W|R|V
    va += sz;
    pa += sz;
    sz = PHY_SIZE - (_sdata - _stext);
    create_mapping(swapper_pg_dir, va, pa, sz, 7);

    // set satp with swapper_pg_dir
    uint64 _satp = (((uint64)(swapper_pg_dir) - PA2VA_OFFSET) >> 12) | (8L << 60);
    csr_write(satp, _satp);

    // flush TLB
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");
    
    // printk("*_stext = %lu, *_srodata = %lu\n", *_stext, *_srodata);
    // *_stext = *_srodata = 0;
    // printk("vm_final done\n");
    return;
}


/**** 创建多级页表映射关系 *****/
/* 不要修改该接口的参数和返回值 */
create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小，单位为字节
    perm 为映射的权限 (即页表项的低 8 位)

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    uint64 index, entry, cnt = 0;
    uint64 *tbl, *tbl2;
    while(cnt < sz) {
        // 一级页表
        index = (va >> 30) & 0x1ff;     // VPN[2]
        if((pgtbl[index] & 1) == 0) {   // V bit == 0, 不存在
            tbl = (uint64 *)kalloc();   // 申请新页
            entry = ((((uint64)tbl - PA2VA_OFFSET) >> 12) << 10) | 1;       // 设置PPN以及mode
            pgtbl[index] = entry;
        }
        tbl = (uint64 *)(((pgtbl[index] >> 10) << 12) + PA2VA_OFFSET);      // 下一级页表的地址
        // 二级页表
        index = (va >> 21) & 0x1ff;     // VPN[1]
        if((tbl[index] & 1) == 0) {
            tbl2 = (uint64 *)kalloc();
            entry = ((((uint64)tbl2 - PA2VA_OFFSET) >> 12) << 10) | 1;
            tbl[index] = entry;
        }
        tbl2 = (uint64 *)(((tbl[index] >> 10) << 12) + PA2VA_OFFSET);
        // 三级页表
        index = (va >> 12) & 0x1ff;     // VPN[2]
        entry = ((pa >> 12) << 10) | perm;
        tbl2[index] = entry;
        cnt += PGSIZE;
        va += PGSIZE;
        pa += PGSIZE;
    }
}