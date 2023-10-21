#include "printk.h"
#include "sbi.h"
#include "defs.h"

extern void test();

int start_kernel() {
    printk("2022");
    printk(" Hello RISC-V\n");
    printk("sstatus = 0x%x\n", csr_read(sstatus));
    
    printk("Before: sscratch = 0x%x\n", csr_read(sscratch));
    csr_write(sscratch, 1);
    printk("After: sscratch = 0x%x\n", csr_read(sscratch));

    test(); // DO NOT DELETE !!!

	return 0;
}
