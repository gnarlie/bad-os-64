#include "rtc.h"

#include "common.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

static int is_update_in_progress() {
    outb(CMOS_ADDR, 0x0a);
    return inb(0x71) & 0x80;
}

static char get_cmos_reg(int reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}


static uint8_t from_bcd(char x) {
    return (x & 0xf) + (x / 16) * 10;
}

static uint32_t read_rtc_internal() {
    while(is_update_in_progress())
        asm volatile ("pause");

    char second = get_cmos_reg(0x00);
    char minute = get_cmos_reg(0x02);
    char hour = get_cmos_reg(0x04);
    char day = get_cmos_reg(0x07);
    char month = get_cmos_reg(0x08);
    uint16_t year = get_cmos_reg(0x09);
    //todo century register ... ?

    char regB = get_cmos_reg(0x0B);
    if (0 == (regB & 0x04)) {
        second = from_bcd(second);
        minute = from_bcd(minute);
        hour = from_bcd(hour);
        day = from_bcd(day);
        month = from_bcd(month);
    }

    if (0 == (regB & 0x02) && (hour & 0x80)) {
        if (hour & 0x80) hour = 0;
        else hour = ((hour & 0x7f) + 12 % 24);
    }

    year += 2000;

    return
        ((hour * 60) + minute) * 60 + second;
}

#include "console.h"

uint32_t read_rtc() {
    uint32_t a, b;
    do {
        a = read_rtc_internal();
        b = read_rtc_internal();
    } while (a != b);

    return a;
}
