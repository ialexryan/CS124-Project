#ifndef BOARD_H
#define BOARD_H

#include "utility.h"

#define BOARD_SIZE 4

#define BOX_HEIGHT 3
#define BOX_WIDTH 6
#define BOX_SPACING 1

void initialize(int board[][BOARD_SIZE]);
int shift(int board[][BOARD_SIZE], shift_direction dir, /* out parameter */ int amount[][BOARD_SIZE]);

#endif /* BOARD_H */
