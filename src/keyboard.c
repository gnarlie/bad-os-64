#include "keyboard.h"
#include "common.h"
#include "console.h"

char scancode[] = "\000\0331234567890-=\b"
                  "\tqwertyuiop[]\n"
                  "\000asdfghjkl;'`"
                  "\000\\zxcvbnm,./\000"
                  "*\000 \000FFFFFFFFFF\000\000"
                  "7894561230."
                  "\000\000\000FF\000\000\000";
char scancodeCap[] =  "\000\000!@#$%^&*()_+\b"
                      "\tQWERTYUIOP{}\n"
                      "\000ASDFGHJKL:\"~"
                      "\000|ZXCVBNM<>?\000"
                      "*\000 \000FFFFFFFFFF\000\000"
                      "7894561230."
                      "\000\000\000FF\000\000\000";

typedef enum KeyStateT { UP, DOWN } KeyState;

static KeyState shift = UP;
static KeyState control = UP;
static KeyState alt = UP;

static uint8_t capsEnabled = 0;
static uint8_t numEnabled = 0;
static uint8_t scrollEnabled = 0;

static void waitForKeyboard() {
   while(inb(0x64) & 0x02)
       ;
}

static void updateLedState() {
    uint8_t state = (capsEnabled ? 4 : 0) |
        (numEnabled ? 2 : 0) |
        (scrollEnabled ? 1 : 0);

   waitForKeyboard();
   outb(0x60, 0xed);
   waitForKeyboard();
   outb(0x60, state);

}

void numLock(KeyState state) {
    if (state == DOWN) {
        numEnabled = !numEnabled;
        updateLedState();
    }
}
void scrollLock(KeyState state) {
    if (state == DOWN) {
        scrollEnabled = !scrollEnabled;
        updateLedState();
    }
}
void capsLock(KeyState state) {
    if (state == DOWN) {
        capsEnabled = !capsEnabled;
        updateLedState();
    }
}

void fkey(uint8_t f, KeyState state)  { }

void keyboard_irq() {
    uint8_t u = inb(0x60);
    KeyState dir = u & 0x80 ? UP : DOWN;
    switch(u & 0x7F) {
        case(0x1d): //left
            control = dir;
            break;
        case(0x38): //left
            alt = dir;
            break;
        case(0x3a):
            capsLock(dir);
            break;
        case(0x3b):
        case(0x3c):
        case(0x3d):
        case(0x3e):
        case(0x3f):
        case(0x40):
        case(0x41):
        case(0x42):
        case(0x43):
        case(0x44):
            fkey(u-0x3b, dir);
            break;
        case(0x45):
            numLock(dir);
            break;
        case(0x46):
            scrollLock(dir);
            break;
        case(0x2a): //left shift
        case(0x36): //right shift
            shift = dir;
            break;
        case(0x00):
        case(0xfc):
        case(0xfd):
        case(0xfe):
        case(0xff):
            console_print_string("keyboard error"); // TODO... stay off console by default
            break;
        default: {
            if (u < sizeof(scancode)) {
                char k = (capsEnabled ^ shift == DOWN) ? scancodeCap[u] : scancode[u];
                if (k) console_put(k);
            }
        }
    }

}

