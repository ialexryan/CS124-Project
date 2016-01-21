#include "video.h"
#include "board.h"

/* This is the address of the VGA text-mode video buffer.  Note that this
 * buffer actually holds 8 pages of text, but only the first page (page 0)
 * will be displayed.
 *
 * Individual characters in text-mode VGA are represented as two adjacent
 * bytes:
 *     Byte 0 = the character value
 *     Byte 1 = the color of the character:  the high nibble is the background
 *              color, and the low nibble is the foreground color
 *
 * See http://wiki.osdev.org/Printing_to_Screen for more details.
 *
 * Also, if you decide to use a graphical video mode, the active video buffer
 * may reside at another address, and the data will definitely be in another
 * format.  It's a complicated topic.  If you are really intent on learning
 * more about this topic, go to http://wiki.osdev.org/Main_Page and look at
 * the VGA links in the "Video" section.
 */

/* TODO:  You can create static variables here to hold video display state,
 *        such as the current foreground and background color, a cursor
 *        position, or any other details you might want to keep track of!
 */

typedef enum {
    update_character = 0x1,
    update_foreground = 0x2,
    update_background = 0x4,
    update_color = update_foreground | update_background,
    update_all = update_character | update_color
} update_options;

int has_option(update_options mask, update_options option) {
    return (mask & option) == option;
}

void apply_advanced(volatile pixel *p, pixel v, update_options options) {
    if (has_option(options, update_character))  p->character  = v.character;
    if (has_option(options, update_foreground)) p->color.foreground = v.color.foreground;
    if (has_option(options, update_background)) p->color.background = v.color.background;
}

void clear_screen_advanced(pixel value, update_options options) {
    volatile pixel *end = VIDEO_BUFFER + VIDEO_SIZE;
    for (volatile pixel *p = VIDEO_BUFFER; p < end; p++) {
        apply_advanced(p, value, options);
   }
}

typedef struct {
    point top_left;
    point bottom_right;
} rectangle;

int rectangle_width(rectangle r) {
    return r.bottom_right.x - r.top_left.x;
}

int rectangle_height(rectangle r) {
    return r.bottom_right.y - r.top_left.y;
}

volatile pixel *apply_offset(volatile pixel *screen, point p) {
    return screen + p.x + (p.y * VIDEO_WIDTH);
}

volatile pixel *apply_offset_vertical(volatile pixel *screen, int y) {
    return apply_offset(screen, (point){ .x = 0, .y = y });
}

volatile pixel *apply_offset_horizontal(volatile pixel *screen, int x) {
    return apply_offset(screen, (point){ .x = x, .y = 0 });
}

point offset(point p, point q) {
    return (point){ .x = p.x + q.x, .y = p.y + q.y };
}

point dir_offset(point p, int z, shift_direction d) {
    switch (d) {
        case left_direction:
            return (point){ .x = p.x - z, .y = p.y };
        case right_direction:
            return (point){ .x = p.x + z, .y = p.y };
        case up_direction:
            return (point){ .x = p.x, .y = p.y - z };
        case down_direction:
            return (point){ .x = p.x, .y = p.y + z };
    }
}

void clear_screen(color_pair color) {
    volatile pixel *end = VIDEO_BUFFER + VIDEO_SIZE;
    for (volatile pixel *p = VIDEO_BUFFER; p < end; p++) {
        p->character  = ' ';
        p->color = color;
   }
}

volatile pixel *draw_string(volatile pixel *p, char *string) {
    while (*string != '\0') {
        p->character = *string;
        string++; p++;
    }
    return p;
}

char rightmost_digit(int number) {
    switch (number % 10) {
        case 0: return '0';
        case 1: return '1';
        case 2: return '2';
        case 3: return '3';
        case 4: return '4';
        case 5: return '5';
        case 6: return '6';
        case 7: return '7';
        case 8: return '8';
        case 9: return '9';
        default: return ' ';
    }
}

volatile pixel *draw_number_rtl(volatile pixel *p /* draws from right to left */, int number) {
    while (number > 0) {
        int lsd = number % 10;
        p->character = rightmost_digit(number);
        number = number / 10; p--;
    }
    return p;
}

void draw_top_decorated_horizontal_line(volatile pixel *screen, int length) {
    if    (length-- > 0) (screen++)->character = 201;
    while (length-- > 1) (screen++)->character = 205;
    screen->character = 187;
}
void draw_bottom_decorated_horizontal_line(volatile pixel *screen, int length) {
    if    (length-- > 0) (screen++)->character = 200;
    while (length-- > 1) (screen++)->character = 205;
    screen->character = 188;
}

void draw_inner_rectangle(volatile pixel *screen, int length, int fill) {
    if    (length-- > 0) (screen++)->character = 186;
    while (length-- > 1) {
        if (fill) (screen++)->character = ' ';
        else screen++;
    }
    (screen++)->character = 186;
}

void color_rectangle(volatile pixel *screen, rectangle r, pixel p, update_options options) {
    screen = apply_offset(screen, r.top_left);
    for (int y = 0; y < rectangle_height(r); y++) {
        for (int x = 0; x < rectangle_width(r); x++) {
            apply_advanced(screen, p, options);
            screen++;
        }
        screen += VIDEO_WIDTH - rectangle_width(r);
    }
}

void draw_rectangle_outline(volatile pixel *screen, rectangle r, int fill) {
    int width  = rectangle_width(r);
    int height = rectangle_height(r);
    if (width <= 0 || height <= 0) return;

    screen = apply_offset(screen, r.top_left);

    if (height-- > 0)    {
        draw_top_decorated_horizontal_line(screen, width);
        screen = apply_offset_vertical(screen, 1);
    }
    while (height-- > 1) {
        draw_inner_rectangle(screen, width, fill);
        screen = apply_offset_vertical(screen, 1);
    }
    draw_bottom_decorated_horizontal_line(screen, width);
}

char color_for_number(int number) {
    switch (number) {
        case 2:
            return CYAN;
        case 4:
            return MAGENTA;
        case 8:
            return RED;
        case 16:
            return BLUE;
        case 32:
            return GREEN;
        case 64:
            return LIGHT_RED;
        case 128:
            return LIGHT_BLUE;
        case 256:
            return LIGHT_MAGENTA;
        case 512:
            return LIGHT_GRAY;
        case 1024:
            return BROWN;
        default:
            return BLACK;
    }
}

void draw_boxed_number(volatile pixel *screen, boxed_number box) {
    if (box.number == 0) return;
    rectangle bounds = { .top_left = box.location, .bottom_right = offset(box.location, (point){ .x = BOX_WIDTH, .y = BOX_HEIGHT }) };
    draw_rectangle_outline(screen, bounds, 1);
    draw_number_rtl(apply_offset(screen, offset(box.location, (point){ .x = BOX_WIDTH - 2, .y = 1 })), box.number);
    color_rectangle(screen, bounds, (pixel){
        .character = '\0',
        .color = (color_pair){ .foreground = color_for_number(box.number), .background = -1 }
    }, update_foreground);
}

void draw_failure_message() {
    draw_string(VIDEO_BUFFER + 11 + VIDEO_WIDTH * 21, "Game over! No more moves. Press ENTER to start a new game.");
    color_rectangle(VIDEO_BUFFER, (rectangle){.top_left = {.x = 10, .y = 21}, .bottom_right = {.x = 70, .y = 22}}, (pixel){
        .character = '\0',
        .color = (color_pair){.foreground = WHITE, .background = RED}
    }, update_color);
}

int frame_offset(point p, animation_descriptor descriptor, int frame) {
    return frame * descriptor.offsets[p.y][p.x] / axis_dimension(get_axis(descriptor.direction));
}

point box_point_to_grid_point(point p) {
    return (point){ .x = p.x * BOX_EFFECTIVE_WIDTH, .y = p.y * BOX_HEIGHT };
}

int frame_count(shift_direction direction) {
    int dimension = axis_dimension(get_axis(direction));
    return dimension * dimension;
}

void draw_board_frame(animation_descriptor descriptor, int frame) {
    int centerx = (VIDEO_WIDTH - BOARD_WIDTH) / 2;
    int centery = (VIDEO_HEIGHT - BOARD_HEIGHT) / 2;
    
    rectangle bounds = { .top_left = {.x = centerx - 2, .y = centery - 1}, .bottom_right = {.x = centerx + 2 + BOARD_WIDTH, .y = centery + 1 + BOARD_HEIGHT}};
    color_rectangle(VIDEO_BUFFER, bounds, (pixel){
        .character = ' ',
        .color = (color_pair){.foreground = DARK_GRAY, .background = WHITE}
    }, update_all);
    draw_rectangle_outline(VIDEO_BUFFER, bounds, 0);
    
    volatile pixel *screen = VIDEO_BUFFER + (VIDEO_WIDTH * centery + centerx);

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            int current_offset = frame_offset((point){ .x = x, .y = y }, descriptor, frame);
            draw_boxed_number(screen, (boxed_number){
                .location = dir_offset(box_point_to_grid_point((point){
                    .x = x,
                    .y = y
                }), current_offset, descriptor.direction),
                .number = descriptor.board[y][x]
            });
        }
    }
}

void draw_board(int board[][BOARD_SIZE]) {
    int centerx = (VIDEO_WIDTH - BOARD_WIDTH) / 2;
    int centery = (VIDEO_HEIGHT - BOARD_HEIGHT) / 2;

    rectangle bounds = { .top_left = {.x = centerx - 2, .y = centery - 1}, .bottom_right = {.x = centerx + 2 + BOARD_WIDTH, .y = centery + 1 + BOARD_HEIGHT}};
    color_rectangle(VIDEO_BUFFER, bounds, (pixel){
        .character = '\0',
        .color = (color_pair){.foreground = DARK_GRAY, .background = WHITE}
    }, update_color);
    draw_rectangle_outline(VIDEO_BUFFER, bounds, 0);

    volatile pixel *screen = VIDEO_BUFFER + (VIDEO_WIDTH * centery + centerx);
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            draw_boxed_number(screen, (boxed_number){
                .location = (point){
                    .x = x * (BOX_WIDTH + BOX_SPACING),
                    .y = y * BOX_HEIGHT
                },
                .number = board[y][x]
            });
        }
    }
}

void init_video(void) {
    clear_screen((color_pair){ .foreground = BLACK, .background = LIGHT_GRAY });
    draw_string(VIDEO_BUFFER, "Play using the arrow keys.");
    draw_string(VIDEO_BUFFER + VIDEO_WIDTH, "Press enter to restart.");
    draw_string(VIDEO_BUFFER + 64, "HIGH SCORE: ");
    draw_number_rtl(VIDEO_BUFFER + 79, high_score);
}
