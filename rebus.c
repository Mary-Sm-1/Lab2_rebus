#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "rebus.h"

#define MAX_WORDS 8
#define MAX_LEN   32
#define ALPH_SIZE 26

/* ---------- global data ---------- */

static char *words[MAX_WORDS];
static int   word_count = 0;
static char *result_word = NULL;

static int letter_digit[ALPH_SIZE];
static int digit_used[10];
static int leading[ALPH_SIZE];
static int letters[ALPH_SIZE];
static int letter_count = 0;

/* ---------- statistics ---------- */

static unsigned long long stat_rec_calls = 0;

void reset_rec_calls(void) { stat_rec_calls = 0; }
unsigned long long get_rec_calls(void) { return stat_rec_calls; }

/* ---------- parsing ---------- */

static int parse(const char *input) {
    char buf[512];
    strncpy(buf, input, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *eq = strchr(buf, '=');
    if (!eq) return 0;
    *eq = '\0';
    char *rhs = eq + 1;

    while (*rhs && isspace((unsigned char)*rhs)) rhs++;
    char *end = rhs + strlen(rhs);
    while (end > rhs && isspace((unsigned char)*(end - 1))) *--end = '\0';
    free(result_word);
    result_word = strdup(rhs);

    for (int i = 0; i < word_count; i++) free(words[i]);
    word_count = 0;

    char *token = strtok(buf, "+");
    while (token) {
        while (*token && isspace((unsigned char)*token)) token++;
        char *e = token + strlen(token);
        while (e > token && isspace((unsigned char)*(e - 1))) *--e = '\0';
        if (*token && word_count < MAX_WORDS)
            words[word_count++] = strdup(token);
        token = strtok(NULL, "+");
    }
    return 1;
}

/* ---------- Optimization 2: sort letters by influence ---------- */
static void sort_letters_by_influence(void) {
    long long priority[ALPH_SIZE] = {0};

    /* Contributions from summands */
    for (int i = 0; i < word_count; i++) {
        int len = (int)strlen(words[i]);
        for (int j = 0; j < len; j++) {
            int c = words[i][j] - 'A';
            if (c < 0 || c >= ALPH_SIZE) continue;
            int power = len - 1 - j;
            priority[c] += (1LL << (10 - power));
        }
    }

    /* Contributions from result */
    int rlen = (int)strlen(result_word);
    for (int j = 0; j < rlen; j++) {
        int c = result_word[j] - 'A';
        if (c < 0 || c >= ALPH_SIZE) continue;
        int power = rlen - 1 - j;
        priority[c] += (1LL << (10 - power));
    }

    /* Bubble sort letters[] by priority descending */
    for (int i = 0; i < letter_count - 1; i++) {
        for (int j = 0; j < letter_count - 1 - i; j++) {
            int c1 = letters[j];
            int c2 = letters[j + 1];
            if (priority[c1] < priority[c2]) {
                int t = letters[j];
                letters[j] = letters[j + 1];
                letters[j + 1] = t;
            }
        }
    }
}


/* ---------- collect unique letters ---------- */

static void collect_letters(void) {
    for (int i = 0; i < ALPH_SIZE; i++) {
        letter_digit[i] = -1;

        leading[i] = 0;
    }
    letter_count = 0;


    for (int i = 0; i < word_count; i++) {
        int len = (int)strlen(words[i]);
        for (int j = 0; j < len; j++) {
            int c = words[i][j] - 'A';
            if (c < 0 || c >= ALPH_SIZE) continue;

            if (letter_digit[c] == -1) {
                letter_digit[c] = -2;
                letters[letter_count++] = c;

            }
        }
        if (len > 1) leading[words[i][0] - 'A'] = 1;
    }


    int len = (int)strlen(result_word);
    for (int j = 0; j < len; j++) {
        int c = result_word[j] - 'A';
        if (c < 0 || c >= ALPH_SIZE) continue;
        if (letter_digit[c] == -1) {

            letter_digit[c] = -2;
            letters[letter_count++] = c;
        }
    }
    if (len > 1) leading[result_word[0] - 'A'] = 1;

    /* Optimization 2: sort letters by influence (units first) */

    sort_letters_by_influence();


    for (int i = 0; i < letter_count; i++)
        letter_digit[letters[i]] = -1;
}

/* ---------- numeric value ---------- */

static long long word_value(const char *w) {

    long long v = 0;
    for (int i = 0; w[i]; i++) {
        int c = w[i] - 'A';
        if (c < 0 || c >= ALPH_SIZE) return -1;
        if (letter_digit[c] < 0) return -1;
        v = v * 10 + letter_digit[c];
    }
    return v;
}

/* ---------- full check ---------- */

static int check_full(void) {
    long long sum = 0;
    for (int i = 0; i < word_count; i++) {
        long long v = word_value(words[i]);
        if (v < 0) return 0;
        sum += v;
    }
    long long r = word_value(result_word);

    if (r < 0) return 0;
    return sum == r;
}

/* ---------- Optimization 1: check units column ---------- */

static int check_last_column(void) {
    long long sum = 0;
    for (int i = 0; i < word_count; i++) {
        int l = (int)strlen(words[i]);
        int c = words[i][l - 1] - 'A';
        if (letter_digit[c] < 0) return 1;

        sum += letter_digit[c];
    }
    int rl = (int)strlen(result_word);
    int rc = result_word[rl - 1] - 'A';
    if (letter_digit[rc] < 0) return 1;
    return (sum % 10) == letter_digit[rc];

}

/* ---------- Optimization 3: fix leading digit of result ---------- */

/* If result is longer than the longest summand (and there are exactly
   2 summands), its leading digit must be 1.
   Returns the letter index (0..25) to fix, or -1 if not applicable. */
static int find_fixed_letter(void) {
    /* Safe: only for 2 summands. With 3+ summands the leading digit
       could be 2 or more (e.g. 999 + 999 + 999 = 2997). */
    if (word_count != 2) return -1;

    int max_len = 0;
    for (int i = 0; i < word_count; i++) {
        int len = (int)strlen(words[i]);
        if (len > max_len) max_len = len;
    }
    int rlen = (int)strlen(result_word);

    if (rlen > max_len) {

        return result_word[0] - 'A';
    }
    return -1;
}


/* ---------- recursive brute force ---------- */

static int solve_rec(int idx, int fixed) {

    stat_rec_calls++;

    if (idx == letter_count) return check_full();


    int c = letters[idx];

    /* Optimization 3: skip the fixed letter */

    if (c == fixed) {
        return solve_rec(idx + 1, fixed);

    }


    for (int d = 0; d <= 9; d++) {

        if (digit_used[d]) continue;
        if (d == 0 && leading[c]) continue;


        letter_digit[c] = d;
        digit_used[d] = 1;
        /* Optimization 1: early pruning on the units column */
        if (check_last_column() && solve_rec(idx + 1, fixed)) return 1;

        digit_used[d] = 0;
        letter_digit[c] = -1;
    }

    return 0;
}


/* ---------- public API ---------- */


int solve(const char *input) {
    reset_rec_calls();


    word_count = 0;
    if (!parse(input)) return 0;
    collect_letters();
    memset(digit_used, 0, sizeof(digit_used));


    /* Optimization 3: fix leading digit of result */
    int fixed = find_fixed_letter();
    if (fixed != -1) {
        letter_digit[fixed] = 1;

        digit_used[1] = 1;
    }

    return solve_rec(0, fixed);
}

void print_solution(void) {
    for (int i = 0; i < word_count; i++) {
        if (i) printf(" + ");
        printf("%lld", word_value(words[i]));
    }
    printf(" = %lld\n", word_value(result_word));
}
