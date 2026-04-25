#pragma once

#include <stdlib.h>

typedef struct {
    int x;
    int y;
} Point;

// only works with null terminated arrays ...
size_t char_arr_len(char **arr) {
    size_t count = 0;
    while(1) {
        char *v = arr[count];
        if (v == NULL) {
            return count;
        }
        count++;
    }
}

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
