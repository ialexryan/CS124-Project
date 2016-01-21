#ifndef KEYBOARD_H
#define KEYBOARD_H

#define MODIFIER_SCANCODE 0xe0
#define LEFT_KEY_SCANCODE 0x4b
#define RIGHT_KEY_SCANCODE 0x4d
#define UP_KEY_SCANCODE 0x48
#define DOWN_KEY_SCANCODE 0x50
#define ENTER_KEY_SCANCODE 0x1c

typedef enum { left, right, up, down, enter } key;

int isemptyqueue();
key dequeue();

void init_keyboard(void);
void keypress_handler(void);

#endif /* KEYBOARD_H */
