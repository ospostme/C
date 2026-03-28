#include "reports.h"

#include <string.h>

double report_total_income(const Budget *b) {
    double total = 0.0;
    for (int i = 0; i < b->transaction_count; i++) {
        if (b->transactions[i].type == TRANSACTION_INCOME) {
            total += b->transactions[i].amount;
        }
    }
    return total;
}

double report_total_expenses(const Budget *b) {
    double total = 0.0;
    for (int i = 0; i < b->transaction_count; i++) {
        if (b->transactions[i].type == TRANSACTION_EXPENSE) {
            total += b->transactions[i].amount;
        }
    }
    return total;
}

double report_monthly_expenses(const Budget *b, int year, int month) {
    double total = 0.0;
    for (int i = 0; i < b->transaction_count; i++) {
        const Transaction *tx = &b->transactions[i];
        /* BUG #4: checks tx->year == month instead of tx->year == year && tx->month == month */
        if (tx->type == TRANSACTION_EXPENSE && tx->year == month && tx->month == month) {
            total += tx->amount;
        }
    }
    (void)year; /* suppress unused warning — part of the bug */
    return total;
}

double report_category_expenses(const Budget *b, const char *category) {
    double total = 0.0;
    for (int i = 0; i < b->transaction_count; i++) {
        const Transaction *tx = &b->transactions[i];
        if (tx->type == TRANSACTION_EXPENSE &&
            strncmp(tx->category, category, MAX_NAME_LEN) == 0) {
            total += tx->amount;
        }
    }
    return total;
}
