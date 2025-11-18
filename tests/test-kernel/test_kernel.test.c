/*
 * Unit Test - Printing
 */

#include "test_kernel_common.h"

void test_kernel_main(void) {
    test_suite_start("Printing");

    test_start("putchar test");
    putchar('O');
    putchar('K');
    putchar('\n');
    test_assert_true(true, "putchar works");

    test_suite_end();
}
