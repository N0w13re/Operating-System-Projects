#include "types.h"
#include "sbi.h"


struct sbiret sbi_ecall(int ext, int fid, uint64 arg0,
			            uint64 arg1, uint64 arg2,
			            uint64 arg3, uint64 arg4,
			            uint64 arg5) 
{
    // unimplemented
	struct sbiret ret;

	__asm__ volatile (
		"mv a7, %2\n"
		"mv a6, %3\n"
		"mv a0, %4\n"
		"mv a1, %5\n"
		"mv a2, %6\n"
		"mv a3, %7\n"
		"mv a4, %8\n"
		"mv a5, %9\n"
		"ecall\n"
		"mv %0, a0\n"
		"mv %1, a1"
		: "=r"(ret.error), "=r"(ret.value)
		: "r"(ext), "r"(fid), "r"(arg0), "r"(arg1), "r"(arg2), 
			"r"(arg3), "r"(arg4), "r"(arg5)
		: "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
	);
}
