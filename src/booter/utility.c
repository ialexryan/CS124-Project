#include "utility.h"

const point null_point = { .x = -1, .y = -1 };

int is_null(point p) {
    return p.x == null_point.x && p.y == null_point.y;
}

#define RAND_MAX 32768
static int rand_state = 0;
int rand() {
    rand_state = rand_state * 1103515245 + 12345;
    return (unsigned int)(rand_state / 65536) % 32768;
}

void srand(unsigned int seed) {
    rand_state = seed;
}
void seed_rand_with_time() {
    int time;
    asm ("rdtsc\n\t"
         "movl %%eax, %0\n\t"
         :"=r"(time)                     /* output */
         :/*no input registers*/         /* input */
         :"%eax"         /* clobbered register */
         );
    srand(time);
}

int random_below(int upper_bound) {
    return rand() % upper_bound;
}

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int min(int lhs, int rhs) {
    return (lhs < rhs) ? lhs : rhs;
}

shift_direction key_to_direction(key k) {
    switch (k) {
        case up_key:
            return up_direction;
        case down_key:
            return down_direction;
        case left_key:
            return left_direction;
        case right_key:
            return right_direction;
        default:
            return (shift_direction)(-1);
    }
}
