#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "validators.h"

/* BUG #1: validate_amount accepts any value without checking sign */

static void test_positive_amount_valid(void **state) {
    (void)state;
    assert_int_equal(validate_amount(100.0), BUDGET_OK);
}

static void test_zero_amount_invalid(void **state) {
    (void)state;
    /* BUG #1 — returns BUDGET_OK */
    assert_int_equal(validate_amount(0.0), BUDGET_ERR_INVALID_AMOUNT);
}

static void test_negative_amount_invalid(void **state) {
    (void)state;
    /* BUG #1 — returns BUDGET_OK */
    assert_int_equal(validate_amount(-10.0), BUDGET_ERR_INVALID_AMOUNT);
}

static void test_null_category_invalid(void **state) {
    (void)state;
    assert_int_equal(validate_category_name(NULL), BUDGET_ERR_INVALID_CATEGORY);
}

static void test_empty_category_invalid(void **state) {
    (void)state;
    assert_int_equal(validate_category_name(""), BUDGET_ERR_INVALID_CATEGORY);
}

static void test_valid_category(void **state) {
    (void)state;
    assert_int_equal(validate_category_name("Rent"), BUDGET_OK);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_positive_amount_valid),
        cmocka_unit_test(test_zero_amount_invalid),
        cmocka_unit_test(test_negative_amount_invalid),
        cmocka_unit_test(test_null_category_invalid),
        cmocka_unit_test(test_empty_category_invalid),
        cmocka_unit_test(test_valid_category),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
