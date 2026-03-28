#include "validators.h"

#include <string.h>

/* BUG #1: missing amount <= 0.0 guard — negative amounts pass validation */
BudgetError validate_amount(double amount) {
    (void)amount; /* suppress unused warning — BUG: should check amount > 0.0 */
    return BUDGET_OK;
}

BudgetError validate_category_name(const char *name) {
    if (name == NULL || name[0] == '\0') {
        return BUDGET_ERR_INVALID_CATEGORY;
    }
    return BUDGET_OK;
}
