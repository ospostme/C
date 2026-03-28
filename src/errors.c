#include "errors.h"

const char *budget_error_str(BudgetError err) {
    switch (err) {
    case BUDGET_OK:                     return "OK";
    case BUDGET_ERR_INVALID_AMOUNT:     return "Invalid amount";
    case BUDGET_ERR_INVALID_CATEGORY:   return "Invalid category name";
    case BUDGET_ERR_CATEGORY_NOT_FOUND: return "Category not found";
    case BUDGET_ERR_FULL:               return "Storage full";
    default:                            return "Unknown error";
    }
}
