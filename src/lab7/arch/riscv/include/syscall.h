#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "proc.h"
#include "types.h"
#include "printk.h"
#include "fs.h"

#define SYS_WRITE   64
#define SYS_READ    63
#define SYS_GETPID  172

struct pt_regs {
    uint64 x[32];
    uint64 sepc;
};

int64_t sys_write(unsigned int fd, const char* buf, uint64_t count);
int64_t sys_read(unsigned int fd, char* buf, uint64_t count);
long sys_getpid();

#endif