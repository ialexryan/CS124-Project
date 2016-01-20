#include "video.h"

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
#define VIDEO_BUFFER ((void *) 0xB8000)


/* TODO:  You can create static variables here to hold video display state,
 *        such as the current foreground and background color, a cursor
 *        position, or any other details you might want to keep track of!
 */


 char color_byte(char fg, char bg) {
    return fg + (bg << 4);
 }

 void clear_screen(char colors) {
    volatile char *video = (volatile char*)VIDEO_BUFFER;
    int i;
    for (i = 0; i < 80*25; i++) {
        *video = ' ';
        video++;
        *video = colors;
        video++;
    }
 }

 void write_char(int x, int y, char colors, char character) {
    volatile char *video = (volatile char*)VIDEO_BUFFER;
    int offset = x + y * 80;
    video += offset * 2;  // Each spot is two bytes, one for the char and one for the color
    *video = character;
    video++;
    *video = colors;
 }


void write_string(int x, int y, char color, char* characters) {
    volatile char *video = (volatile char*)VIDEO_BUFFER;
    int offset = x + y * 80;
    video += offset * 2;
    while (*characters != '\0') {
        *video = *characters;
        characters++;
        video++;
        *video = color;
        video++;
    }
}

void init_video(void) {
    /* TODO:  Do any video display initialization you might want to do, such
     *        as clearing the screen, initializing static variable state, etc.
     */
    clear_screen(color_byte(RED, GREEN));
    write_string(40, 17, color_byte(CYAN, MAGENTA), "Hello, world!");
}
