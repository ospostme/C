#include <gtest/gtest.h>

extern "C" {
#include "budget.h"
}

class BudgetTest : public ::testing::Test {
  protected:
    Budget b;

    void SetUp() override {
        budget_init(&b);
        budget_add_category(&b, "Food", 500.0);
        budget_add_category(&b, "Salary", 0.0);
    }
};

/* BUG #2: balance adds income and expenses instead of subtracting expenses */
TEST_F(BudgetTest, BalanceSubtractsExpenses) {
    budget_add_transaction(&b, TRANSACTION_INCOME, "Salary", 1000.0, 2024, 1, 1);
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 200.0, 2024, 1, 2);
    /* Expected: 1000 - 200 = 800; BUG returns 1000 + 200 = 1200 */
    EXPECT_DOUBLE_EQ(budget_balance(&b), 800.0);
}

TEST_F(BudgetTest, BalanceWithNoTransactionsIsZero) {
    EXPECT_DOUBLE_EQ(budget_balance(&b), 0.0);
}

/* BUG #3: category over limit uses > instead of >= */
TEST_F(BudgetTest, CategoryExactlyAtLimitIsOverLimit) {
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 500.0, 2024, 1, 1);
    /* 500.0 == limit 500.0 — should be over, BUG returns false */
    EXPECT_EQ(budget_category_over_limit(&b, "Food"), 1);
}

TEST_F(BudgetTest, CategoryBelowLimitIsNotOver) {
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 499.0, 2024, 1, 1);
    EXPECT_EQ(budget_category_over_limit(&b, "Food"), 0);
}

TEST_F(BudgetTest, CategoryAboveLimitIsOver) {
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 600.0, 2024, 1, 1);
    EXPECT_EQ(budget_category_over_limit(&b, "Food"), 1);
}

TEST_F(BudgetTest, AddTransactionToMissingCategoryFails) {
    EXPECT_EQ(budget_add_transaction(&b, TRANSACTION_EXPENSE, "Unknown", 50.0, 2024, 1, 1),
              BUDGET_ERR_CATEGORY_NOT_FOUND);
}
