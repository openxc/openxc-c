#include <check.h>
#include <stdint.h>
#include "openxc/openxc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void setup() {
}

Suite* suite(void) {
    Suite* s = suite_create("openxc");
    TCase *tc_base_tests = tcase_create("base");
    tcase_add_checked_fixture(tc_base_tests, setup, NULL);
    suite_add_tcase(s, tc_base_tests);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = suite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
