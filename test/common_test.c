#include "common.h"

#include "tinytest.h"

TEST(strncmp) {
    ASSERT_INT_EQUALS(0, strncmp("a", "a", 1));
    ASSERT_INT_EQUALS(0, strncmp("ac", "ab", 1));
    ASSERT_INT_EQUALS(-1, strncmp("a", "b", 1));
    ASSERT_INT_EQUALS(1, strncmp("b", "a", 1));
    ASSERT_INT_EQUALS(0, strncmp("a", "a", 10));
    ASSERT_INT_EQUALS(-1, strncmp("a", "ab", 10));
}

TEST(memcmp) {
    ASSERT_INT_EQUALS(0, memcmp("a", "a", 1));
    ASSERT_INT_EQUALS(0, memcmp("ac", "ab", 1));
    ASSERT_INT_EQUALS(-1, memcmp("a", "b", 1));
    ASSERT_INT_EQUALS(1, memcmp("b", "a", 1));
}

TEST(strnchr) {
    ASSERT_EQUALS(' ', *strnchr("123 ", ' ', 50));
    ASSERT_EQUALS(NULL, strnchr("123 ", ' ', 2));
    ASSERT_EQUALS(NULL, strnchr("123", ' ', 12));
}

TEST(more_string) {
    ASSERT_INT_EQUALS(1, strlen("a"));
    ASSERT_INT_EQUALS(0, strlen(""));

    char buf_odd[] = "abc";
    strrev(buf_odd, buf_odd + 2);
    ASSERT_STRING_EQUALS("cba", buf_odd);

    char buf_even[] = "abcd";
    strrev(buf_even, buf_even + 3);
    ASSERT_STRING_EQUALS("dcba", buf_even);

    char b32[32];
    to_str(1, b32);
    ASSERT_STRING_EQUALS("1", b32);
    to_str(10, b32);
    ASSERT_STRING_EQUALS("10", b32);
    to_str(100001, b32);
    ASSERT_STRING_EQUALS("100001", b32);
}
