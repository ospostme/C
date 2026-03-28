#include <criterion/criterion.h>

#include "validators.h"

/* BUG #1: validate_amount does not reject non-positive amounts */

Test(validators, positive_amount_is_valid) {
    cr_assert_eq(validate_amount(100.0), BUDGET_OK,
                 "Expected BUDGET_OK for positive amount");
}

Test(validators, zero_amount_should_be_invalid) {
    /* BUG #1 — returns BUDGET_OK instead of BUDGET_ERR_INVALID_AMOUNT */
    cr_assert_eq(validate_amount(0.0), BUDGET_ERR_INVALID_AMOUNT,
                 "Zero amount must be rejected");
}

Test(validators, negative_amount_should_be_invalid) {
    /* BUG #1 — returns BUDGET_OK instead of BUDGET_ERR_INVALID_AMOUNT */
    cr_assert_eq(validate_amount(-1.0), BUDGET_ERR_INVALID_AMOUNT,
                 "Negative amount must be rejected");
}

Test(validators, valid_category_name) {
    cr_assert_eq(validate_category_name("Groceries"), BUDGET_OK);
}

Test(validators, empty_category_name_is_invalid) {
    cr_assert_eq(validate_category_name(""), BUDGET_ERR_INVALID_CATEGORY);
}

Test(validators, null_category_name_is_invalid) {
    cr_assert_eq(validate_category_name(NULL), BUDGET_ERR_INVALID_CATEGORY);
}
