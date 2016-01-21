#include "ports.h"
#include "handlers.h"
#include "keyboard.h"
#include "interrupts.h"

/* This is the IO port of the PS/2 controller, where the keyboard's scan
 * codes are made available.  Scan codes can be read as follows:
 *
 *     unsigned char scan_code = inb(KEYBOARD_PORT);
 *
 * Most keys generate a scan-code when they are pressed, and a second scan-
 * code when the same key is released.  For such keys, the only difference
 * between the "pressed" and "released" scan-codes is that the top bit is
 * cleared in the "pressed" scan-code, and it is set in the "released" scan-
 * code.
 *
 * A few keys generate two scan-codes when they are pressed, and then two
 * more scan-codes when they are released.  For example, the arrow keys (the
 * ones that aren't part of the numeric keypad) will usually generate two
 * scan-codes for press or release.  In these cases, the keyboard controller
 * fires two interrupts, so you don't have to do anything special - the
 * interrupt handler will receive each byte in a separate invocation of the
 * handler.
 *
 * See http://wiki.osdev.org/PS/2_Keyboard for details.
 */
#define KEYBOARD_PORT 0x60


/* TODO:  You can create static variables here to hold keyboard state.
 *        Note that if you create some kind of circular queue (a very good
 *        idea, you should declare it "volatile" so that the compiler knows
 *        that it can be changed by exceptional control flow.
 *
 *        Also, don't forget that interrupts can interrupt *any* code,
 *        including code that fetches key data!  If you are manipulating a
 *        shared data structure that is also manipulated from an interrupt
 *        handler, you might want to disable interrupts while you access it,
 *        so that nothing gets mangled...
 */


static unsigned int modified = 0;

// citation: this is a standard circular queue implementation from a data structures book
static int front = -1;
static int rear = -1;
static int capacity = 50;
static key array[50];

int isemptyqueue() {
   return(front==-1);
}
int isfullqueue() {
   return((rear+1)%capacity==rear);
}
int queuesize() {
   return(capacity-rear+front+1)%capacity;
}
static void enqueue(key x) {
   int ints_on = disable_interrupts();
   if(isfullqueue())
      return; // queue overflow
   else{
      rear=(rear+1)%capacity;
      array[rear]=x;
      if(front==-1) {
         front=rear;
      }
   }
   if (ints_on) enable_interrupts();
}
key dequeue() {
   int ints_on = disable_interrupts();
   key data=0;
   if(isemptyqueue()) {
      return -1; // queue underflow
   }
   else {
      data=array[front];
      if(front==rear)
         front=rear=-1;
      else
         front=(front+1)%capacity;
   }
   if (ints_on) enable_interrupts();
   return data;
}


void init_keyboard(void) {
    install_interrupt_handler(KEYBOARD_INTERRUPT, irq1_handler);
}

void keypress_handler(void) {
    unsigned char scan_code = inb(KEYBOARD_PORT);

    if (modified) {
        switch(scan_code) {
            case LEFT_KEY_SCANCODE:
                enqueue(left);
                break;
            case RIGHT_KEY_SCANCODE:
                enqueue(right);
                break;
            case UP_KEY_SCANCODE:
                enqueue(up);
                break;
            case DOWN_KEY_SCANCODE:
                enqueue(down);
                break;
        }
        modified = 0;
    } else {
        switch(scan_code) {
            case MODIFIER_SCANCODE:
                modified = 1;
                break;
            case ENTER_KEY_SCANCODE:
                enqueue(enter);
                break;
        }
    }
}
