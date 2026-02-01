#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

/* Configuration */
#define NUM_ACCOUNTS 10
#define NUM_THREADS 8
#define TRANSFERS_PER_THREAD 1000
#define INITIAL_BALANCE 1000

/* Account structure */
typedef struct {
    int id;
    int balance;
    pthread_mutex_t lock;
} account_t;

/* Error helpers */
static inline void die(const char *msg) {
    perror(msg);
    exit(1);
}

static inline void die2(const char *msg, const char *detail) {
    fprintf(stderr, "%s: %s\n", msg, detail);
    exit(1);
}

/* Initialize all accounts with locks */
static inline void init_accounts(account_t accounts[], int n, int initial_balance) {
    for (int i = 0; i < n; i++) {
        accounts[i].id = i;
        accounts[i].balance = initial_balance;
        if (pthread_mutex_init(&accounts[i].lock, NULL) != 0) {
            die("pthread_mutex_init");
        }
    }
}

/* Print all account balances */
static inline void print_balances(account_t accounts[], int n) {
    printf("Account balances:\n");
    for (int i = 0; i < n; i++) {
        printf("  Account %d: $%d\n", accounts[i].id, accounts[i].balance);
    }
}

/* Verify the total balance (money should not be created or destroyed) */
static inline int verify_total(account_t accounts[], int n, int expected_total) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        total += accounts[i].balance;
    }
    printf("Total balance: $%d (expected: $%d)\n", total, expected_total);
    if (total != expected_total) {
        fprintf(stderr, "ERROR: Balance mismatch! Money was lost or created.\n");
        return 0;
    }
    return 1;
}

/* Get current time in nanoseconds */
static inline uint64_t now_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        die("clock_gettime");
    }
    return (uint64_t)ts.tv_sec * 1000000000LL + (uint64_t)ts.tv_nsec;
}

/* Get current time in microseconds */
static inline uint64_t now_us(void) {
    return now_ns() / 1000;
}

#endif /* COMMON_H */
