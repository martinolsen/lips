#include <check.h>

#include "lexer.h"

START_TEST(eine testen) {
    const char *s = "(+ 1 2)";
    lexer_t *lexer = lexer_init(s, strlen(s));
} END_TEST
