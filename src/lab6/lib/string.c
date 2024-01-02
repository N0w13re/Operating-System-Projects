#include "string.h"

void *memset(void *dst, int c, uint64 n) {
    char *cdst = (char *)dst;
    for (uint64 i = 0; i < n; ++i)
        cdst[i] = c;

    return dst;
}

char *memcpy(char *dst, char *src, uint64 n) {
    for (uint64 i = 0; i < n; ++i)
        dst[i] = src[i];
    
    return dst;
}