#include "budget.h"

#include "validators.h"

#include <string.h>

void budget_init(Budget *b) {
    memset(b, 0, sizeof(*b));
}

BudgetError budget_add_category(Budget *b, const char *name, double limit) {
    BudgetError err = validate_category_name(name);
    if (err != BUDGET_OK) {
        return err;
    }
    if (b->category_count >= MAX_CATEGORIES) {
        return BUDGET_ERR_FULL;
    }
    strncpy(b->categories[b->category_count].name, name, MAX_NAME_LEN - 1);
    b->categories[b->category_count].budget_limit = limit;
    b->category_count++;
    return BUDGET_OK;
}

BudgetError budget_add_transaction(Budget *b, TransactionType type, const char *category,
                                   double amount, int year, int month, int day) {
    BudgetError err = validate_amount(amount);
    if (err != BUDGET_OK) {
        return err;
    }
    err = validate_category_name(category);
    if (err != BUDGET_OK) {
        return err;
    }

    /* Check category exists */
    int found = 0;
    for (int i = 0; i < b->category_count; i++) {
        if (strncmp(b->categories[i].name, category, MAX_NAME_LEN) == 0) {
            found = 1;
            break;
        }
    }
    if (!found) {
        return BUDGET_ERR_CATEGORY_NOT_FOUND;
    }

    if (b->transaction_count >= MAX_TRANSACTIONS) {
        return BUDGET_ERR_FULL;
    }

    Transaction *tx = &b->transactions[b->transaction_count];
    tx->type        = type;
    strncpy(tx->category, category, MAX_NAME_LEN - 1);
    tx->amount = amount;
    tx->year   = year;
    tx->month  = month;
    tx->day    = day;
    b->transaction_count++;
    return BUDGET_OK;
}

double budget_balance(const Budget *b) {
    double total = 0.0;
    for (int i = 0; i < b->transaction_count; i++) {
        const Transaction *tx = &b->transactions[i];
        /* BUG #2: income and expenses both ADD — should subtract expenses */
        total += tx->amount;
    }
    return total;
}

int budget_category_over_limit(const Budget *b, const char *name) {
    double limit = 0.0;
    int    has_limit = 0;

    for (int i = 0; i < b->category_count; i++) {
        if (strncmp(b->categories[i].name, name, MAX_NAME_LEN) == 0) {
            limit     = b->categories[i].budget_limit;
            has_limit = (limit > 0.0);
            break;
        }
    }
    if (!has_limit) {
        return 0;
    }

    double spent = 0.0;
    for (int i = 0; i < b->transaction_count; i++) {
        const Transaction *tx = &b->transactions[i];
        if (tx->type == TRANSACTION_EXPENSE &&
            strncmp(tx->category, name, MAX_NAME_LEN) == 0) {
            spent += tx->amount;
        }
    }
    /* BUG #3: strict > instead of >= means on-limit is not flagged as over */
    return spent > limit;
}
