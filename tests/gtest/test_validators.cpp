#include <gtest/gtest.h>

extern "C" {
#include "validators.h"
}

/* BUG #1: validate_amount accepts negative amounts */
TEST(Validators, PositiveAmountIsValid) {
    EXPECT_EQ(validate_amount(100.0), BUDGET_OK);
}

TEST(Validators, ZeroAmountShouldBeInvalid) {
    /* BUG #1 — returns BUDGET_OK instead of BUDGET_ERR_INVALID_AMOUNT */
    EXPECT_EQ(validate_amount(0.0), BUDGET_ERR_INVALID_AMOUNT);
}

TEST(Validators, NegativeAmountShouldBeInvalid) {
    /* BUG #1 — returns BUDGET_OK instead of BUDGET_ERR_INVALID_AMOUNT */
    EXPECT_EQ(validate_amount(-50.0), BUDGET_ERR_INVALID_AMOUNT);
}

TEST(Validators, ValidCategoryName) {
    EXPECT_EQ(validate_category_name("Food"), BUDGET_OK);
}

TEST(Validators, EmptyCategoryNameIsInvalid) {
    EXPECT_EQ(validate_category_name(""), BUDGET_ERR_INVALID_CATEGORY);
}

TEST(Validators, NullCategoryNameIsInvalid) {
    EXPECT_EQ(validate_category_name(nullptr), BUDGET_ERR_INVALID_CATEGORY);
}
