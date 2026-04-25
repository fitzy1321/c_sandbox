#pragma once

#include <stdlib.h>

typedef struct {
    int x;
    int y;
} Point;


void point_dosomething(Point *p) {
    p->x++;
    p->y--;
    return;
}

void secure_zero_memory(void *ptr, size_t len) {
    if (ptr != NULL && len > 0) {
        volatile unsigned char *vptr = (volatile unsigned char *)ptr;
        while(len--) {
            vptr[len] = 0;
        }
    }
}
