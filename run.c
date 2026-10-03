#include <stdio.h>
#include <string.h>
#include <time.h>
#include "rebus.h"

static const char *PUZZLES[] = {
    "SEND + MORE = MONEY",
    "BE + BE = MOO",
    "BIG + CAT = LION",
    "TWO + TWO = FOUR",
    "ELEVEN + NINE + FIVE + FIVE = THIRTY"
};

static void run_one(const char *puzzle) {
    clock_t t0 = clock();
    int ok = solve(puzzle);
    clock_t t1 = clock();

    printf("=== %s ===\n", puzzle);

    if (!ok) {
        printf("No solution\n\n");
        return;
    }
    print_solution();
    printf("Time: %.6f s\n", (double)(t1 - t0) / CLOCKS_PER_SEC);
    printf("Recursive calls: %llu\n\n", get_rec_calls());
}

int main(int argc, char **argv) {
    if (argc >= 2) {
        /* run only the puzzle(s) passed on the command line */
        for (int i = 1; i < argc; i++) {
            run_one(argv[i]);
        }
    }


    /* no arguments: run all built-in puzzles */
    int n = (int)(sizeof(PUZZLES) / sizeof(PUZZLES[0]));

    for (int i = 0; i < n; i++) {

        run_one(PUZZLES[i]);

    }

    return 0;
}
