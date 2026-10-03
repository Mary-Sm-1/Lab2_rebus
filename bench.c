#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "rebus.h"

#define RUNS 20

static const char *PUZZLES[] = {
    "SEND + MORE = MONEY",
    "BE + BE = MOO",
    "BIG + CAT = LION",
    "TWO + TWO = FOUR",
    "ELEVEN + NINE + FIVE + FIVE = THIRTY"
};

static double run_once(const char *input, unsigned long long *rec_calls) {
    clock_t t0 = clock();
    int ok = solve(input);
    clock_t t1 = clock();
    if (!ok) {
        fprintf(stderr, "No solution for: %s\n", input);
        exit(1);
    }
    *rec_calls = get_rec_calls();
    return (double)(t1 - t0) / CLOCKS_PER_SEC;
}

int main(void) {
    int n = (int)(sizeof(PUZZLES) / sizeof(PUZZLES[0]));

    printf("%-42s %12s %16s\n", "PUZZLE", "MIN TIME(s)", "REC CALLS");
    printf("%-42s %12s %16s\n",
           "------------------------------------------",
           "------------",
           "----------------");

    for (int p = 0; p < n; p++) {
        double best_t = -1.0;
        unsigned long long best_rc = 0;

        for (int r = 0; r < RUNS; r++) {
            unsigned long long rc = 0;
            double t = run_once(PUZZLES[p], &rc);
            if (best_t < 0 || t < best_t) {
                best_t = t;
                best_rc = rc;
            }
        }
        printf("%-42s %12.6f %16llu\n",
               PUZZLES[p], best_t, best_rc);
    }
    return 0;
}
