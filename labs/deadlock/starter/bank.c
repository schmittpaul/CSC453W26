#define _DEFAULT_SOURCE
#include "common.h"
#include <getopt.h>
#include <inttypes.h>

/* Global statistics */
struct stats {
    uint64_t successful_transfers;
    uint64_t failed_transfers;
    uint64_t deadlock_detections;
    uint64_t retries;
} stats = {0, 0, 0, 0};

/* Thread worker data */
struct worker_args {
    account_t *accounts;
    int num_accounts;
    int mode;
    int transfers;
};

/* ====== TRANSFER FUNCTION DECLARATIONS ====== */

/* Part 1: Naive transfer (provided - will deadlock) */
static void transfer_naive(account_t *from, account_t *to, int amount);

/* Part 2: Timeout-based detection (students implement) */
static int transfer_timeout(account_t *from, account_t *to, int amount);

/* Part 3A: Ordered lock acquisition (students implement) */
static void transfer_ordered(account_t *from, account_t *to, int amount);

/* Part 3B: Try-lock with backoff (students implement) */
static int transfer_trylock(account_t *from, account_t *to, int amount);

/* ====== PART 1: NAIVE TRANSFER (PROVIDED) ====== */
/*
 * This function demonstrates a VULNERABLE transfer that WILL DEADLOCK.
 *
 * Why it deadlocks:
 * - Thread A locks Account 1, then tries to lock Account 2
 * - Thread B locks Account 2, then tries to lock Account 1
 * - Both threads wait forever (circular wait)
 *
 * This satisfies all 4 deadlock conditions:
 * 1. Mutual exclusion: Accounts can only be locked by one thread
 * 2. Hold-and-wait: Threads hold one lock while waiting for another
 * 3. No preemption: Can't take locks away forcefully
 * 4. Circular wait: A waits for resource held by B, B waits for A
 */
static void transfer_naive(account_t *from, account_t *to, int amount) {
    /* Lock source account */
    pthread_mutex_lock(&from->lock);

    /* Simulate some work */
    usleep(1);

    /* Lock destination account (DANGER: may deadlock here!) */
    pthread_mutex_lock(&to->lock);

    /* Perform transfer */
    from->balance -= amount;
    to->balance += amount;

    /* Unlock in reverse order */
    pthread_mutex_unlock(&to->lock);
    pthread_mutex_unlock(&from->lock);

    __sync_fetch_and_add(&stats.successful_transfers, 1);
}

/*
 * Try to acquire a mutex with a timeout by repeatedly trying (polling).
 *
 * TODO: Implement this helper function.
 * It should:
 * - Loop until timeout_ms has elapsed
 * - Use pthread_mutex_trylock() to attempt to acquire lock (non-blocking)
 * - Sleep 10ms between attempts with usleep(10 * 1000)
 * - Return 0 on success, ETIMEDOUT if timeout expires
 *
 */
static int mutex_trylock_timed(pthread_mutex_t *lock, int timeout_ms) {
    /* TODO: Implement timed lock using trylock and polling */
    return ETIMEDOUT;  /* Placeholder */
}

/* ====== PART 2: TIMEOUT-BASED DETECTION ====== */
/*
 * TODO: Implement deadlock detection using timeouts.
 *
 * Use the mutex_trylock_timed() helper to acquire locks with a timeout.
 * If a timeout expires, it signals a potential deadlock.
 *
 * Strategy:
 * - Try to lock 'from' with 100ms timeout
 * - If timeout, increment deadlock_detections and return -1 (retry)
 * - Try to lock 'to' with 100ms timeout
 * - If second lock times out, RELEASE first lock, then return -1
 * - If both succeed, perform transfer and return 0
 *
 * Key point: Must not hold one lock while waiting for another on timeout!
 */
static int transfer_timeout(account_t *from, account_t *to, int amount) {
    /* Try to lock 'from' with 100ms timeout */
    int ret1 = mutex_trylock_timed(&from->lock, 100);
    if (ret1 == ETIMEDOUT) {
        /* TODO: Increment deadlock detection counter */
        return -1;  /* Retry */
    }

    /* Simulate some work */
    usleep(1);

    /* Try to lock 'to' with 100ms timeout */
    int ret2 = mutex_trylock_timed(&to->lock, 100);
    if (ret2 == ETIMEDOUT) {
        /* TODO: Release 'from' lock before retrying */
        /* TODO: Increment deadlock detection counter */
        return -1;  /* Retry */
    }

    /* Both locks acquired - perform transfer */
    from->balance -= amount;
    to->balance += amount;

    /* Unlock in reverse order */
    pthread_mutex_unlock(&to->lock);
    pthread_mutex_unlock(&from->lock);

    __sync_fetch_and_add(&stats.successful_transfers, 1);
    return 0;
}

/* ====== PART 3A: ORDERED LOCK ACQUISITION (TODO) ====== */
/*
 * TODO: Implement deadlock prevention by always acquiring locks in order.
 *
 * Key insight: If all threads acquire locks in the same order,
 * a circular wait can never form.
 *
 * Strategy:
 * - Compare account IDs
 * - Always lock the lower-ID account first
 * - Then lock the higher-ID account
 * - Perform transfer (being careful about which is from/to)
 * - Unlock in reverse order
 *
 * Questions to think about:
 * - Which deadlock condition does this prevent?
 * - What if amount is negative (reverse the direction)?
 */
static void transfer_ordered(account_t *from, account_t *to, int amount) {
    /* TODO: Implement ordered locking */
    /*
     * Hint: Determine which account to lock first based on ID
     * account_t *first, *second;
     * if (from->id < to->id) {
     *     first = from;
     *     second = to;
     * } else {
     *     first = to;
     *     second = from;
     * }
     */

    /* TODO: Lock in order, perform transfer, unlock in reverse */

    __sync_fetch_and_add(&stats.successful_transfers, 1);
}

/* ====== PART 3B: TRY-LOCK WITH BACKOFF (TODO) ====== */
/*
 * TODO: Implement deadlock prevention using non-blocking locks.
 *
 * Instead of blocking on a lock, use pthread_mutex_trylock()
 * which returns immediately with EBUSY if the lock is unavailable.
 *
 * If we can't acquire both locks, release any we got and retry later.
 * Use random backoff to avoid thundering herd.
 *
 * Strategy:
 * - Try to lock 'from' (non-blocking)
 * - If EBUSY, return -1 to retry
 * - Try to lock 'to' (non-blocking)
 * - If EBUSY, release 'from' and return -1 to retry
 * - If both succeed, perform transfer
 * - Use usleep(rand() % 1000) for random backoff between retries
 *
 * Questions to think about:
 * - Which deadlock condition does this prevent?
 * - Why is random backoff important?
 */
static int transfer_trylock(account_t *from, account_t *to, int amount) {
    /* TODO: Implement try-lock with backoff */

    return -1;  /* TODO: implement this function */
}

/* ====== THREAD WORKER ====== */
static void *worker(void *arg) {
    struct worker_args *args = (struct worker_args *)arg;

    for (int i = 0; i < args->transfers; i++) {
        /* Pick random source and destination (must be different) */
        int from_id = rand() % args->num_accounts;
        int to_id = rand() % args->num_accounts;
        if (from_id == to_id) {
            i--;  /* Skip this iteration */
            continue;
        }

        account_t *from = &args->accounts[from_id];
        account_t *to = &args->accounts[to_id];
        int amount = 1 + (rand() % 10);  /* Transfer 1-10 dollars */

        /* Call appropriate transfer function based on mode */
        int retry_count = 0;
        int max_retries = 1000;

        if (args->mode == 0) {  /* NAIVE */
            transfer_naive(from, to, amount);
        } else if (args->mode == 1) {  /* TIMEOUT */
            while (transfer_timeout(from, to, amount) != 0) {
                __sync_fetch_and_add(&stats.retries, 1);
                if (++retry_count > max_retries) {
                    __sync_fetch_and_add(&stats.failed_transfers, 1);
                    break;
                }
                usleep(10 * 1000);  /* 10ms delay between retries */
            }
        } else if (args->mode == 2) {  /* ORDERED */
            transfer_ordered(from, to, amount);
        } else if (args->mode == 3) {  /* TRYLOCK */
            while (transfer_trylock(from, to, amount) != 0) {
                __sync_fetch_and_add(&stats.retries, 1);
                if (++retry_count > max_retries) {
                    __sync_fetch_and_add(&stats.failed_transfers, 1);
                    break;
                }
                usleep(rand() % 1000000);  /* Random backoff 0-999999 µs */
            }
        }
    }

    return NULL;
}

/* ====== MAIN ====== */
int main(int argc, char *argv[]) {
    int num_threads = NUM_THREADS;
    int transfers_per_thread = TRANSFERS_PER_THREAD;
    int mode = 0;  /* 0=naive, 1=timeout, 2=ordered, 3=trylock */

    /* Parse command line arguments */
    int opt;
    while ((opt = getopt(argc, argv, "t:n:m:")) != -1) {
        switch (opt) {
        case 't':
            num_threads = atoi(optarg);
            break;
        case 'n':
            transfers_per_thread = atoi(optarg);
            break;
        case 'm':
            mode = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-t threads] [-n transfers] [-m mode]\n", argv[0]);
            fprintf(stderr, "  mode: 0=naive, 1=timeout, 2=ordered, 3=trylock\n");
            exit(1);
        }
    }

    printf("=== Deadlock Lab ===\n");
    printf("Mode: ");
    if (mode == 0) printf("NAIVE (will deadlock)\n");
    else if (mode == 1) printf("TIMEOUT (detect deadlock)\n");
    else if (mode == 2) printf("ORDERED (prevent deadlock)\n");
    else if (mode == 3) printf("TRYLOCK (prevent deadlock)\n");
    else {
        fprintf(stderr, "Invalid mode\n");
        exit(1);
    }
    printf("Threads: %d, Transfers per thread: %d\n\n", num_threads, transfers_per_thread);

    /* Initialize accounts */
    account_t accounts[NUM_ACCOUNTS];
    init_accounts(accounts, NUM_ACCOUNTS, INITIAL_BALANCE);

    /* Seed random number generator */
    srand(time(NULL));

    /* Create worker threads */
    pthread_t threads[num_threads];
    struct worker_args args = {
        .accounts = accounts,
        .num_accounts = NUM_ACCOUNTS,
        .mode = mode,
        .transfers = transfers_per_thread
    };

    uint64_t t0 = now_ns();

    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, worker, &args) != 0) {
            die("pthread_create");
        }
    }

    /* Wait for all threads to complete */
    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            die("pthread_join");
        }
    }

    uint64_t t1 = now_ns();
    double elapsed_sec = (t1 - t0) / 1e9;

    /* Verify results */
    printf("\n=== Results ===\n");
    printf("Elapsed time: %.3f seconds\n", elapsed_sec);
    printf("Successful transfers: %" PRIu64 "\n", stats.successful_transfers);
    printf("Failed transfers: %" PRIu64 "\n", stats.failed_transfers);
    printf("Deadlock detections: %" PRIu64 "\n", stats.deadlock_detections);
    printf("Retries: %" PRIu64 "\n", stats.retries);
    printf("Throughput: %.2f transfers/sec\n\n", stats.successful_transfers / elapsed_sec);

    /* Check account balances */
    print_balances(accounts, NUM_ACCOUNTS);
    printf("\n");

    int expected_total = NUM_ACCOUNTS * INITIAL_BALANCE;
    if (!verify_total(accounts, NUM_ACCOUNTS, expected_total)) {
        printf("FAIL: Balance check failed!\n");
        return 1;
    }

    printf("SUCCESS: All transfers completed and balances verified.\n");
    return 0;
}
