#include <check.h>
#include <stdio.h>

#include "../src/input/input.h"
#include "suites.h"

/* Reads one action from a pipe that holds `keys` (a closed pipe, so the end of input follows). */
static input_action_t read_action(const char* keys) {
    FILE* in = tmpfile();
    ck_assert_ptr_nonnull(in);
    fputs(keys, in);
    rewind(in);

    input_t* input = NULL;
    ck_assert_int_eq(input_create(in, &input), INPUT_STATUS_OK);
    input_action_t action = input_read(input);
    input_destroy(&input);

    fclose(in);
    return action;
}

START_TEST(test_input_maps_keys_to_actions) {
    ck_assert_int_eq(read_action("h"), INPUT_ACTION_HORIZONTAL_TOGGLE);
    ck_assert_int_eq(read_action("v"), INPUT_ACTION_VERTICAL_TOGGLE);
    ck_assert_int_eq(read_action("q"), INPUT_ACTION_QUIT);
    ck_assert_int_eq(read_action("x"), INPUT_ACTION_NONE);
}
END_TEST

START_TEST(test_input_end_of_file_is_quit) {
    ck_assert_int_eq(read_action(""), INPUT_ACTION_QUIT);
}
END_TEST

Suite* input_suite(void) {
    Suite* suite = suite_create("input");
    TCase* tcase = tcase_create("core");
    tcase_add_test(tcase, test_input_maps_keys_to_actions);
    tcase_add_test(tcase, test_input_end_of_file_is_quit);
    suite_add_tcase(suite, tcase);
    return suite;
}
