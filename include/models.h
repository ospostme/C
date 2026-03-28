#ifndef MODELS_H
#define MODELS_H

#include <stddef.h>

#define MAX_NAME_LEN     64
#define MAX_CATEGORIES   20
#define MAX_TRANSACTIONS 200

typedef enum {
    TRANSACTION_INCOME,
    TRANSACTION_EXPENSE,
} TransactionType;

typedef struct {
    char   name[MAX_NAME_LEN];
    double budget_limit; /* 0 = no limit */
} Category;

typedef struct {
    TransactionType type;
    char            category[MAX_NAME_LEN];
    double          amount;
    int             year;
    int             month;
    int             day;
} Transaction;

#endif /* MODELS_H */
