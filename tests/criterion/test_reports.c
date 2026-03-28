#include <criterion/criterion.h>

#include "budget.h"
#include "reports.h"

static Budget b;

void setup_reports(void) {
    budget_init(&b);
    budget_add_category(&b, "Food", 0.0);
    budget_add_category(&b, "Salary", 0.0);
    budget_add_category(&b, "Rent", 0.0);

    budget_add_transaction(&b, TRANSACTION_INCOME, "Salary", 3000.0, 2024, 3, 1);
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Rent", 1500.0, 2024, 3, 5);
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 200.0, 2024, 3, 10);
    /* April — must not appear in March results */
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 100.0, 2024, 4, 1);
}

TestSuite(reports, .init = setup_reports);

Test(reports, total_income) {
    cr_assert_float_eq(report_total_income(&b), 3000.0, 1e-9);
}

Test(reports, total_expenses) {
    cr_assert_float_eq(report_total_expenses(&b), 1800.0, 1e-9);
}

/* BUG #4: monthly filter compares year field against month value */

Test(reports, monthly_expenses_only_current_month) {
    /* March 2024: 1500 + 200 = 1700; BUG returns 0 */
    double result = report_monthly_expenses(&b, 2024, 3);
    cr_assert_float_eq(result, 1700.0, 1e-9,
                       "March expenses should be 1700.00, got %.2f", result);
}

Test(reports, monthly_expenses_other_month_is_zero) {
    cr_assert_float_eq(report_monthly_expenses(&b, 2024, 2), 0.0, 1e-9);
}

Test(reports, category_expenses) {
    cr_assert_float_eq(report_category_expenses(&b, "Food"), 300.0, 1e-9);
}
