#ifndef ERRORS_H
#define ERRORS_H

typedef enum {
    BUDGET_OK = 0,
    BUDGET_ERR_INVALID_AMOUNT,
    BUDGET_ERR_INVALID_CATEGORY,
    BUDGET_ERR_CATEGORY_NOT_FOUND,
    BUDGET_ERR_FULL,
} BudgetError;

const char *budget_error_str(BudgetError err);

#endif /* ERRORS_H */
