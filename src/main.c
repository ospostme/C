#include "budget.h"
#include "reports.h"

#include <stdio.h>

int main(void) {
    Budget b;
    budget_init(&b);

    budget_add_category(&b, "Food", 500.0);
    budget_add_category(&b, "Rent", 1500.0);
    budget_add_category(&b, "Salary", 0.0);

    budget_add_transaction(&b, TRANSACTION_INCOME, "Salary", 3000.0, 2024, 3, 1);
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Rent", 1500.0, 2024, 3, 5);
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 200.0, 2024, 3, 10);
    budget_add_transaction(&b, TRANSACTION_EXPENSE, "Food", 150.0, 2024, 3, 20);

    printf("Balance:        %.2f\n", budget_balance(&b));
    printf("Total income:   %.2f\n", report_total_income(&b));
    printf("Total expenses: %.2f\n", report_total_expenses(&b));
    printf("March expenses: %.2f\n", report_monthly_expenses(&b, 2024, 3));
    printf("Food expenses:  %.2f\n", report_category_expenses(&b, "Food"));
    printf("Rent over limit: %s\n", budget_category_over_limit(&b, "Rent") ? "yes" : "no");
    printf("Food over limit: %s\n", budget_category_over_limit(&b, "Food") ? "yes" : "no");

    return 0;
}
