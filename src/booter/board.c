#include "board.h"
#include "utility.h"

#define NUM_STARTING_PIECES 2

int is_available(int board[][BOARD_SIZE], point p) {
    return board[p.y][p.x] == 0;
}

void place(int board[][BOARD_SIZE], point p, int v) {
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
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            board[x][y] = 0;
        }
    }
    int count = 0;
    while (count < 2) {
        count += add_random_box(board);
    }
}

# define shift_dir(direction) \
int shift_ ## direction(int board[][BOARD_SIZE], /* out parameter */ int offset[][BOARD_SIZE]) { \
    int board_mutated = 0; \
    for (int line = 0; line < BOARD_SIZE; line++) { \
        int next_unoccupied_index = loop_start; \
        int previous_mergable = 0; \
        for (int index = loop_start; loop_check(index); index = index + loop_dir) { \
            int value = access(board, line, index); \
            if (value != 0) { \
                if (access(board, line, index) == previous_mergable) { /* See if we can merge */ \
                    /* We can merge into the index before the next open index (e.g. the last filled index) */ \
                    int merge_index = next_unoccupied_index - loop_dir; \
                    \
                    /* Record distance travelled, flipping sign if we're traversing backwards  */ \
                    access(offset,line,index) = (index - merge_index) * loop_dir; \
                    \
                    /* Merge into the previous tile */ \
                    access(board, line, merge_index) *= 2; \
                    access(board, line, index) = 0; \
                    board_mutated = 1; \
                    \
                    /* Don't allow double merges */ \
                    previous_mergable = 0; \
                } \
                else  { /* Just shift into an empty space */ \
                    /* Record distance travelled, flipping sign if we're traversing backwards */ \
                    access(offset, line, index) = (index - next_unoccupied_index) * loop_dir; \
                    \
                    /* Shift as far as possible */ \
                    if (next_unoccupied_index != index) { \
                        swap(&access(board, line, next_unoccupied_index), &access(board,line, index)); \
                        board_mutated = 1; \
                    } \
                    next_unoccupied_index = (next_unoccupied_index + loop_dir); \
                    \
                    /* Allow potential future merge */ \
                    previous_mergable = value; \
                } \
            } \
        } \
    } \
return board_mutated; \
}

// Forward iteration
# define loop_start 0
# define loop_check(x) ((x) < BOARD_SIZE)
# define loop_dir 1

// Left shift
# define access(array, line, index) ((array)[line][index])
shift_dir(left)
# undef access

// Up shift
# define access(array, line, index) ((array)[index][line])
shift_dir(up)
# undef access

# undef loop_start
# undef loop_check
# undef loop_dir

// Backwards iteration
# define loop_start (BOARD_SIZE - 1)
# define loop_check(x) ((x) >= 0)
# define loop_dir (-1)

// Right shift
# define access(array, line, index) ((array)[line][index])
shift_dir(right)
# undef access

// Down shift
# define access(array, line, index) ((array)[index][line])
shift_dir(down)
# undef access

# undef loop_start
# undef loop_check
# undef loop_dir

typedef enum {
    vertical_axis,
    horizontal_axis
} shift_axis;

shift_axis get_axis(shift_direction direction) {
    switch (direction) {
        case up_direction:
        case down_direction:
            return vertical_axis;
        case left_direction:
        case right_direction:
            return horizontal_axis;
    }
}

shift_axis opposite_axis(shift_axis axis) {
    switch (axis) {
        case vertical_axis:
            return horizontal_axis;
        case horizontal_axis:
            return vertical_axis;
    }
}

int axis_dimension(shift_axis axis) {
    switch (axis) {
        case vertical_axis:
            return BOX_HEIGHT;
        case horizontal_axis:
            return BOX_WIDTH;
    }
}

int shift(int board[][BOARD_SIZE], shift_direction dir, /* out parameter */ int offset[][BOARD_SIZE]) {
    switch (dir) {
        case up_direction:    return shift_up(board, offset);
        case down_direction:  return shift_down(board, offset);
        case left_direction:  return shift_left(board, offset);
        case right_direction: return shift_right(board, offset);
    }
}

void copy_board(int from[][BOARD_SIZE], int to[][BOARD_SIZE]) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            to[y][x] = from[y][x];
        }
    }
}

int move_available(int board[][BOARD_SIZE]) {
    if (num_available(board) > 0) return  1;

    // Check if any adjacent spaces are the same number and can merge.
    for (int y = 0; y < BOARD_SIZE; y++) for (int x = 0; x < BOARD_SIZE; x++) {
        if (x + 1 < BOARD_SIZE) if (board[y][x] == board[y][x + 1]) return 1;
        if (x - 1 >= 0) if (board[y][x] == board[y][x - 1]) return 1;
        if (y + 1 < BOARD_SIZE) if (board[y][x] == board[y + 1][x]) return 1;
        if (y - 1 >= 0) if (board[y][x] == board[y - 1][x]) return 1;
    }
    return 0;
}
