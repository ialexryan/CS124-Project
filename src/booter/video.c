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

//typedef enum {
//    update_character = 0x1,
//    update_foreground = 0x2,
//    update_background = 0x4,
//    update_color = update_foreground | update_background,
//    update_all = update_character | update_color
//} update_options;

//bool has_option(update_options mask, update_options option) {
//    return (mask & option) == option;
//}

//void clear_screen(pixel value, update_options options) {
//    volatile pixel *end = VIDEO_BUFFER + VIDEO_SIZE;
//    for (volatile pixel *p = VIDEO_BUFFER; p < end; p++) {
//        if (has_option(options, update_character))  pixel->character  = value.character;
//        if (has_option(options, update_foreground)) pixel->foreground = value.foreground;
//        if (has_option(options, update_background)) pixel->background = value.background;
//   }
//}

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

volatile pixel *offset(volatile pixel *p, int x, int y) {
    return p + x + (y * VIDEO_WIDTH);
}

void draw_number_box(volatile pixel *p, int number) {
    if (number == 0) return;
    draw_string(p, "*----*");
    draw_string(draw_string(draw_string(offset(p, 0, 1), "|"), "    "), "|");
    draw_number_rtl(offset(p, 4, 1), number);
    draw_string(offset(p, 0, 2), "*----*");
}

void draw_board(volatile pixel *p, int board[][BOARD_SIZE]) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            draw_number_box(offset(p, x * 7, y * 3), board[y][x]);
        }
    }
}

//
//volatile pixel *get_pixel(int x, int y) {
//    return VIDEOBUFFER + x + y * VIDEO_HEIGHT;
//}
//
//void write_string(int x, int y, char *string, col) {
//    volatile char *pixel = get_pixel(x, y);
//    while (*string != '\0') {
//        *pixel
//        pixel++; string++;
//    }
//    
//    int offset = x + y * 80;
//    video += offset * 2;
//    while (*characters != '\0') {
//        *video = *characters;
//        characters++;
//        video++;
//        *video = colors.raw_value;
//        video++;
//    }
//}

void init_video(void) {
    clear_screen((color_pair){ .foreground = RED, .background = WHITE });
}
