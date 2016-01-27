//
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
    int high_score = current_score(board);
    init_video(high_score);
    draw_board(board);
    
    /* Loop forever, so that we don't fall back into the bootloader code. */
    while (1) {
        if (!isemptyqueue()) {
            key k = dequeue();
            if (k == enter_key) {
                for (int *b = *board; b < *board + BOARD_SIZE * BOARD_SIZE; b++) *b = 0;
                initialize(board);
                init_video(high_score);
                draw_board(board);
                continue;
            } else {
                shift_direction direction = key_to_direction(k);

                // Setup animation
                copy_board(board, descriptor.board);
                descriptor.direction = direction;

                // Only add a box if the pieces actually move
                if (shift(board, direction, descriptor.offsets)) {
                    add_random_box(board);
                    int score = current_score(board);
                    if (score > high_score) high_score = score;
                    init_video(high_score);
                }
            }
            
            int num_frames = frame_count(descriptor.direction);
            int incr = get_axis(descriptor.direction) == horizontal_axis ? 12 : 2;
            int frame = 0;
            while (frame <= num_frames) {
                draw_board_frame(descriptor, frame);
                sleep(30);

                if (frame == num_frames) break;
                else frame = min(frame + incr, num_frames);
            }
            draw_board(board);
            
            if (!move_available(board)) {
                draw_failure_message();
            }
        }
    }
}
