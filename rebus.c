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


    /* sort letters alphabetically for deterministic order */

    for (int i = 0; i < letter_count - 1; i++) {

        for (int j = 0; j < letter_count - 1 - i; j++) {
            if (letters[j] > letters[j + 1]) {
                int t = letters[j];

                letters[j] = letters[j + 1];

                letters[j + 1] = t;
            }

        }
    }

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

/* ---------- recursive brute force ---------- */

static int solve_rec(int idx) {
    stat_rec_calls++;

    if (idx == letter_count) return check_full();

    int c = letters[idx];
    for (int d = 0; d <= 9; d++) {
        if (digit_used[d]) continue;
        if (d == 0 && leading[c]) continue;

        letter_digit[c] = d;
        digit_used[d] = 1;
        if (solve_rec(idx + 1)) return 1;
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
    return solve_rec(0);
}

void print_solution(void) {
    for (int i = 0; i < word_count; i++) {
        if (i) printf(" + ");
        printf("%lld", word_value(words[i]));
    }
    printf(" = %lld\n", word_value(result_word));
}
