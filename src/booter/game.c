
#include "board.h"
#include "video.h"
#include "interrupts.h"
#include "keyboard.h"
#include "timer.h"
#include "utility.h"

/* This is the entry-point for the game! */
void c_start(void) {
    /* TODO:  You will need to initialize various subsystems here.  This
     *        would include the interrupt handling mechanism, and the various
     *        systems that use interrupts.  Once this is done, you can call
     *        enable_interrupts() to start interrupt handling, and go on to
     *        do whatever else you decide to do!
     */

    seed_rand_with_time();
    init_interrupts();
    init_keyboard();
    init_timer();
    enable_interrupts();

    int board[BOARD_SIZE][BOARD_SIZE] = { };
    animation_descriptor descriptor = { };
    initialize(board);
    init_video();

    /* Loop forever, so that we don't fall back into the bootloader code. */
    while (1) {
        if (!isemptyqueue()) {
            shift_direction direction = key_direction(dequeue());

            // Setup animation
            copy_board(board, descriptor.board);
            descriptor.direction = direction;

            // Only add a box if the pieces actually move
            if (shift(board, direction, descriptor.offsets)) {
                add_random_box(board);
            }

            init_video();
            int board_width = (BOX_WIDTH + BOX_SPACING) * BOARD_SIZE - BOX_SPACING;
            int board_height = BOX_HEIGHT * BOARD_SIZE;
            int centerx = (VIDEO_WIDTH - board_width) / 2;
            int centery = (VIDEO_HEIGHT - board_height) / 2;

            draw_board(VIDEO_BUFFER + (VIDEO_WIDTH * centery + centerx), board);
        }
    }
}
