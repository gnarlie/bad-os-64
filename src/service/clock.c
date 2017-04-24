#include "clock.h"

#include "common.h"
#include "rtc.h"
#include "interrupt.h"
#include "task.h"
#include "entry.h"

static void update_clock(void* unused) {
    static uint32_t time;
    uint32_t now = gettime();

    char * clock = (char*) 0xb8000 + 2 * (80 - 8);

    if (time != now) {
        time = now;

        uint32_t hr = time / 3600;
        clock[0] = (hr / 10) + '0';
        clock[2] = (hr % 10) + '0';
        clock[4] = ':';
        uint32_t min = time / 60 % 60;
        clock[6] = (min / 10) + '0';
        clock[8] = (min % 10) + '0';
        clock[10] = ':';
        uint32_t sec = time % 60;
        clock[12] = (sec / 10) + '0';
        clock[14] = (sec % 10) + '0';
    }
}

static void timer_irq(registers_t* regs, void *update_task) {
    read_rtc();
    task_enqueue((Task*)update_task);
}

static void user_task(void * fn) {
    call_user_function(fn);
}

void init_clock() {
    //enable the timer and display a clock
    Task * update_task = task_alloc(user_task, update_clock);
    add_ref(update_task);
    register_interrupt_handler(IRQ0, timer_irq, update_task);

}
