#include "board.h"

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

typedef struct {
    int x;
    int y;
} point;

point null_point = { .x = -1, .y = -1 };

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

int is_null(point p) {
    return p.x == null_point.x && p.y == null_point.y;
}

int is_available(int board[][BOARD_SIZE], point p) {
    return board[p.y][p.x] == 0;
}

int place(int board[][BOARD_SIZE], point p, int v) {
    board[p.y][p.x] = v;
}

int num_available(int board[][BOARD_SIZE]) {
    int count = 0;
    for (int y = 0; y < BOARD_SIZE; y++)
    for (int x = 0; x < BOARD_SIZE; x++) {
        if (is_available(board, (point){ .x = x, .y = y } )) count++;
    }
    return count;
}

point indexed_available_box(int board[][BOARD_SIZE], int i) {
    for (int y = 0; y < BOARD_SIZE; y++)
    for (int x = 0; x < BOARD_SIZE; x++) {
        if (is_available(board, (point){ .x = x, .y = y })) {
            if (i == 0) return (point){ .x = x, .y = y };
            i--;
        }
    }
    return null_point;
}

point random_available_box(int board[][BOARD_SIZE]) {
    return indexed_available_box(board, random_below(num_available(board)));
}

// bool for success
int add_random_box(int board[][BOARD_SIZE]) {
    point p = random_available_box(board);
    if (is_null(p)) return 0;
    place(board, p, random_below(10) == 0 ? 4 : 2);
    return 1;
}

void initialize(int board[][BOARD_SIZE]) {
    int count = 0;
    while (count < 2) {
        count += add_random_box(board);
    }
}
