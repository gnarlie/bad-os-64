#include "net/ntox.h"

#include "../tinytest.h"

void convertShorts() {
    ASSERT_EQUALS(0x1234, ntos(0x3412));
    ASSERT_EQUALS(0x8005, ntos(0x0580));
}

void convertInts() {
    ASSERT_EQUALS(0xa8c00203, ntol(0x0302c0a8));
}

void convertAll() {
    convertShorts();
    convertInts();
}


