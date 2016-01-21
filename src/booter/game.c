
#include "video.h"
#include "board.h"
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
    draw_board(board);

    /* Loop forever, so that we don't fall back into the bootloader code. */
    while (1) {
        if (!isemptyqueue()) {
            key k = dequeue();
            if (k == enter_key) {
                initialize(board);
            } else {
                shift_direction direction = key_to_direction(k);

                // Setup animation
                copy_board(board, descriptor.board);
                descriptor.direction = direction;

                // Only add a box if the pieces actually move
                if (shift(board, direction, descriptor.offsets)) {
                    add_random_box(board);
                    update_high_score(board);
                }
            }
            
            int num_frames = frame_count(descriptor.direction);
            for (int frame = 0; frame < 100*num_frames; frame++) {
                init_video();
                draw_board_frame(descriptor, frame);
                
//                sleep(1000);
            }
            
            if (!move_available(board)) {
                draw_failure_message();
            }
        }
    }
}
