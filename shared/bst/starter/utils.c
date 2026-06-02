/*******************************************************************************
 * Name        : utils.c
 * Author      : William Ee
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include "utils.h"

int cmpr_int(void* a, void* b) {
    int x = *(int*)a;
    int y = *(int*)b;

    if (x < y) return -1;
    if (x > y) return 1;

    return 0;
}

int cmpr_float(void* a, void* b) {
    float x = *(float*)a;
    float y = *(float*)b;

    if (x < y) return -1;
    if (x > y) return 1;

    return 0;
}

void print_int(void* data) {
    printf("%d ", *(int*)data);
}

void print_float(void* data) {
    printf("%f ", *(float*)data);
}
