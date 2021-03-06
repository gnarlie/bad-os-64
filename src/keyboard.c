#include "keyboard.h"
#include "common.h"
#include "console.h"
#include "task.h"
#include "interrupt.h"

static char scancode[] = "\000\0331234567890-=\b"
                  "\tqwertyuiop[]\n"
                  "\000asdfghjkl;'`"
                  "\000\\zxcvbnm,./\000"
                  "*\000 \000FFFFFFFFFF\000\000"
                  "7894561230."
                  "\000\000\000FF\000\000\000";
static char scancodeCap[] =  "\000\000!@#$%^&*()_+\b"
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

static void numLock(KeyState state) {
    if (state == DOWN) {
        numEnabled = !numEnabled;
        updateLedState();
    }
}
static void scrollLock(KeyState state) {
    if (state == DOWN) {
        scrollEnabled = !scrollEnabled;
        updateLedState();
    }
}

static void capsLock(KeyState state) {
    if (state == DOWN) {
        capsEnabled = !capsEnabled;
        updateLedState();
    }
}

void fkey(uint8_t f, KeyState state)  { if (state == DOWN) asm ("int $3"); }

///// buffer handling
#define KeyboardBufferSize 32
typedef struct KeyboardBufferT {
    char data[KeyboardBufferSize];
    int r;
    int w;
} KeyboardBuffer;

static void buffer_init(KeyboardBuffer* b) {
    b->r = b->w = 0; // KeyboardBufferSize - 1;
}

static int buffer_has_data(KeyboardBuffer* b) {
    return b->r != b->w;
}

static char buffer_pop(KeyboardBuffer* b) {
    if (b->r + 1 == KeyboardBufferSize)
        b->r = 0;
    else
        b->r++;

    return b->data[b->r];
}

static void buffer_push(KeyboardBuffer* b, char c) {
    if (b->w + 1 == KeyboardBufferSize)
        b->w = 0;
    else
        b->w++;
    b->data[b->w] = c;
}

/////

static KeyboardBuffer buffer;


static void read_key(char u) {
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
            warn("keyboard error\n"); // TODO... stay off console by default
            break;
        default: {
            if (u < sizeof(scancode)) {
                char k = (capsEnabled ^ shift == DOWN) ? scancodeCap[u] : scancode[u];
                if (k) console_put(k);
            }
        }
    }
}

static Task * readKeybd;

static void read_keys(void* unused) {
    char c;
    while(buffer_has_data(&buffer)) {
        read_key(buffer_pop(&buffer));
    }
}

void init_keyboard() {
    readKeybd = task_alloc(read_keys, NULL);
    buffer_init(&buffer);
    add_ref(readKeybd);

    register_interrupt_handler(IRQ1, keyboard_irq, 0);
}

void keyboard_irq(void* ptr) {
    uint8_t u = inb(0x60);
    buffer_push(&buffer, u);
    task_enqueue(readKeybd);
}

