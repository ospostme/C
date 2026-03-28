#ifndef BUDGET_H
#define BUDGET_H

#include "errors.h"
#include "models.h"

typedef struct {
    Category    categories[MAX_CATEGORIES];
    int         category_count;
    Transaction transactions[MAX_TRANSACTIONS];
    int         transaction_count;
} Budget;

void        budget_init(Budget *b);
BudgetError budget_add_category(Budget *b, const char *name, double limit);
BudgetError budget_add_transaction(Budget *b, TransactionType type, const char *category,
                                   double amount, int year, int month, int day);
double      budget_balance(const Budget *b);
int         budget_category_over_limit(const Budget *b, const char *name);

#endif /* BUDGET_H */
