#ifndef VIDEO_H
#define VIDEO_H

#include "board.h"

/* Available colors from the 16-color palette used for EGA and VGA, and
 * also for text-mode VGA output.
 */
#define BLACK          0
#define BLUE           1
#define GREEN          2
#define CYAN           3
#define RED            4
#define MAGENTA        5
#define BROWN          6
#define LIGHT_GRAY     7
#define DARK_GRAY      8
#define LIGHT_BLUE     9
#define LIGHT_GREEN   10
#define LIGHT_CYAN    11
#define LIGHT_RED     12
#define LIGHT_MAGENTA 13
#define YELLOW        14
#define WHITE         15

typedef union {
    struct {
        char foreground : 4;
        char background : 4;
    };
    char raw_value;
} color_pair;

typedef struct {
    char character;
    color_pair color;
} pixel;

#define VIDEO_WIDTH 80
#define VIDEO_HEIGHT 25
#define VIDEO_SIZE ((VIDEO_WIDTH) * (VIDEO_HEIGHT))

#define VIDEO_BUFFER ((volatile pixel*) 0xB8000)

void init_video(void);
void clear_screen(color_pair color);
void draw_board(volatile pixel *p, int board[][BOARD_SIZE]);

typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int offsets[BOARD_SIZE][BOARD_SIZE];
    shift_direction direction;
} animation_descriptor;

#endif /* VIDEO_H */
