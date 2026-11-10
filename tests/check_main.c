#include <check.h>
#include <stdlib.h>

#include "suites.h"

int main(void) {
    SRunner* runner = srunner_create(car_suite());
    srunner_add_suite(runner, road_suite());
    srunner_add_suite(runner, config_suite());
    srunner_add_suite(runner, view_suite());

    /* Set CK_FORK=no in the environment to run in-process (e.g. under valgrind/gdb). */
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
