#ifndef REPORTS_H
#define REPORTS_H

#include "budget.h"

double report_total_income(const Budget *b);
double report_total_expenses(const Budget *b);

/* Returns sum of expenses for the given year+month (1-12). */
double report_monthly_expenses(const Budget *b, int year, int month);

/* Returns sum of expenses for the named category. */
double report_category_expenses(const Budget *b, const char *category);

#endif /* REPORTS_H */
