#ifndef BOARD_H
#define BOARD_H

#include "utility.h"

#define BOARD_SIZE 4

#define BOX_HEIGHT 3
#define BOX_WIDTH 6
#define BOX_EFFECTIVE_WIDTH ((BOX_WIDTH) + (BOX_SPACING))
#define BOX_SPACING 1

#define BOARD_WIDTH (BOX_WIDTH + BOX_SPACING) * BOARD_SIZE - BOX_SPACING
#define BOARD_HEIGHT BOX_HEIGHT * BOARD_SIZE

void initialize(int board[][BOARD_SIZE]);
int shift(int board[][BOARD_SIZE], shift_direction dir, /* out parameter */ int amount[][BOARD_SIZE]);
void copy_board(int from[][BOARD_SIZE], int to[][BOARD_SIZE]);
int add_random_box(int board[][BOARD_SIZE]);
int move_available(int board[][BOARD_SIZE]);
void update_high_score(int board[][BOARD_SIZE]);

typedef enum {
    vertical_axis,
    horizontal_axis
} shift_axis;

shift_axis get_axis(shift_direction direction);
shift_axis opposite_axis(shift_axis axis);
int axis_dimension(shift_axis axis);
int current_score(int board[][BOARD_SIZE]);

#endif /* BOARD_H */
