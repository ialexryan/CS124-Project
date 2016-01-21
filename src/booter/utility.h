#ifndef UTILITY_H
#define UTILITY_H

#include "keyboard.h"

typedef struct {
    int x;
    int y;
} point;

extern const point null_point;

int is_null(point p);
void swap(int *a, int *b);
int min(int lhs, int rhs);
int random_below(int upper_bound);

void seed_rand_with_time();

typedef enum {
    up_direction,
    down_direction,
    left_direction,
    right_direction
} shift_direction;

shift_direction key_to_direction(key k);

#endif /* UTILITY_H */
