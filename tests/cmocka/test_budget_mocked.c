/*
 * Demonstrates cmocka mocking: we intercept validate_amount to force
 * a controlled return value — simulating an error from the validator
 * without actually changing the validator's real implementation.
 *
 * In a real project, this pattern is used to mock external I/O, malloc,
 * database calls, or any function seam where you want to inject failures.
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "budget.h"

/* ---------------------------------------------------------------------------
 * Mock shim: replaces validate_amount at link time for this test binary.
 * The real validators.c is NOT linked — CMakeLists.txt links mock_budget
 * against src objects minus validators.c, plus this file.
 *
 * cmocka's will_return / mock_type() mechanism works as a FIFO queue:
 *   will_return(validate_amount, BUDGET_ERR_INVALID_AMOUNT)
 * causes the next call to mock_validate_amount to dequeue that value.
 * --------------------------------------------------------------------------- */
BudgetError validate_amount(double amount) {
    (void)amount;
    return (BudgetError)mock();
}

BudgetError validate_category_name(const char *name) {
    (void)name;
    return (BudgetError)mock();
}

/* ---------------------------------------------------------------------------
 * Tests
 * --------------------------------------------------------------------------- */

static Budget b;

static int setup(void **state) {
    (void)state;
    budget_init(&b);
    /* Stub two calls used by budget_add_category (validate_category_name) */
    will_return(validate_category_name, BUDGET_OK);
    budget_add_category(&b, "Food", 500.0);
    will_return(validate_category_name, BUDGET_OK);
    budget_add_category(&b, "Salary", 0.0);
    return 0;
}

/* Scenario: validate_amount signals an error — add_transaction must propagate it */
static void test_add_transaction_propagates_amount_error(void **state) {
    (void)state;
    will_return(validate_amount, BUDGET_ERR_INVALID_AMOUNT);
    BudgetError err =
        budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", -50.0, 2024, 1, 1);
    assert_int_equal(err, BUDGET_ERR_INVALID_AMOUNT);
}

/* Scenario: validate_category_name signals an error after amount passes */
static void test_add_transaction_propagates_category_error(void **state) {
    (void)state;
    will_return(validate_amount, BUDGET_OK);
    will_return(validate_category_name, BUDGET_ERR_INVALID_CATEGORY);
    BudgetError err =
        budget_add_transaction(&b, TRANSACTION_EXPENSE, "", 50.0, 2024, 1, 1);
    assert_int_equal(err, BUDGET_ERR_INVALID_CATEGORY);
}

/* BUG #2 visible even via mock: balance is wrong with mocked validators */
static void test_balance_subtracts_expenses_via_mock(void **state) {
    (void)state;
    /* Stub the validators so add_transaction succeeds */
    will_return(validate_amount, BUDGET_OK);
    will_return(validate_category_name, BUDGET_OK);
    budget_add_transaction(&b, TRANSACTION_INCOME, "Salary", 1000.0, 2024, 1, 1);

    will_return(validate_amount, BUDGET_OK);
    will_return(validate_category_name, BUDGET_OK);
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 200.0, 2024, 1, 2);

    /* BUG #2: returns 1200 instead of 800 */
    assert_float_equal(budget_balance(&b), 800.0, 1e-9);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup(test_add_transaction_propagates_amount_error, setup),
        cmocka_unit_test_setup(test_add_transaction_propagates_category_error, setup),
        cmocka_unit_test_setup(test_balance_subtracts_expenses_via_mock, setup),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
