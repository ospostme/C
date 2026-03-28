#include <gtest/gtest.h>

extern "C" {
#include "budget.h"
#include "reports.h"
}

class ReportsTest : public ::testing::Test {
  protected:
    Budget b;

    void SetUp() override {
        budget_init(&b);
        budget_add_category(&b, "Food", 0.0);
        budget_add_category(&b, "Salary", 0.0);
        budget_add_category(&b, "Rent", 0.0);

        budget_add_transaction(&b, TRANSACTION_INCOME, "Salary", 3000.0, 2024, 3, 1);
        budget_add_transaction(&b, TRANSACTION_EXPENSE, "Rent", 1500.0, 2024, 3, 5);
        budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 200.0, 2024, 3, 10);
        /* Different month — should not appear in March report */
        budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 100.0, 2024, 4, 1);
    }
};

TEST_F(ReportsTest, TotalIncome) {
    EXPECT_DOUBLE_EQ(report_total_income(&b), 3000.0);
}

TEST_F(ReportsTest, TotalExpenses) {
    EXPECT_DOUBLE_EQ(report_total_expenses(&b), 1800.0);
}

/* BUG #4: monthly filter uses tx->year == month instead of tx->year == year */
TEST_F(ReportsTest, MonthlyExpensesOnlyCurrentMonth) {
    /* March 2024 expenses: 1500 + 200 = 1700; April 100 excluded */
    /* BUG: year check is wrong — result will be 0.0 */
    EXPECT_DOUBLE_EQ(report_monthly_expenses(&b, 2024, 3), 1700.0);
}

TEST_F(ReportsTest, MonthlyExpensesDifferentMonthIsZero) {
    EXPECT_DOUBLE_EQ(report_monthly_expenses(&b, 2024, 2), 0.0);
}

TEST_F(ReportsTest, CategoryExpenses) {
    EXPECT_DOUBLE_EQ(report_category_expenses(&b, "Food"), 300.0);
}

TEST_F(ReportsTest, CategoryExpensesUnknownIsZero) {
    EXPECT_DOUBLE_EQ(report_category_expenses(&b, "Gym"), 0.0);
}
