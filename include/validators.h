#ifndef VALIDATORS_H
#define VALIDATORS_H

#include "errors.h"

BudgetError validate_amount(double amount);
BudgetError validate_category_name(const char *name);

#endif /* VALIDATORS_H */
