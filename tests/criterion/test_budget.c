#include <criterion/criterion.h>

#include "budget.h"

static Budget b;

void setup_budget(void) {
    budget_init(&b);
    budget_add_category(&b, "Food", 500.0);
    budget_add_category(&b, "Salary", 0.0);
}

/* BUG #2: balance adds expenses instead of subtracting */

TestSuite(budget, .init = setup_budget);

Test(budget, balance_with_no_transactions) {
    cr_assert_float_eq(budget_balance(&b), 0.0, 1e-9);
}

Test(budget, balance_subtracts_expenses) {
    budget_add_transaction(&b, TRANSACTION_INCOME, "Salary", 1000.0, 2024, 1, 1);
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 200.0, 2024, 1, 2);
    /* BUG #2: returns 1200 instead of 800 */
    cr_assert_float_eq(budget_balance(&b), 800.0, 1e-9,
                       "Balance must subtract expenses (got %.2f)", budget_balance(&b));
}

/* BUG #3: over-limit check uses > instead of >= */

Test(budget, category_exactly_at_limit_is_over) {
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 500.0, 2024, 1, 1);
    /* BUG #3: 500 > 500 is false, so returns 0 */
    cr_assert_eq(budget_category_over_limit(&b, "Food"), 1,
                 "Spending equal to limit must be flagged as over");
}

Test(budget, category_below_limit_is_not_over) {
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 400.0, 2024, 1, 1);
    cr_assert_eq(budget_category_over_limit(&b, "Food"), 0);
}

Test(budget, add_transaction_unknown_category_fails) {
    cr_assert_eq(
        budget_add_transaction(&b, TRANSACTION_EXPENSE, "Unknown", 50.0, 2024, 1, 1),
        BUDGET_ERR_CATEGORY_NOT_FOUND);
}
