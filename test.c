#include <CUnit/Basic.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "lisp.h"
#include "list.h"

#define ADD_TEST(t, s) do { if(NULL == CU_add_test(suite, s, t)) { \
        CU_cleanup_registry(); return CU_get_error(); } } while(0);

#define ASSERT_PRINT(i, o) CU_ASSERT_STRING_EQUAL_FATAL(print(i), o);

/*****************************
 ** List suite              **
 *****************************/

void test_list() {
    list_t *list = list_new();

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);

    const char *a = "A";
    const char *b = "B";

    list_push(list, list_node_new(a));
    list_push(list, list_node_new(b));

    CU_ASSERT_PTR_EQUAL_FATAL(list_pop(list)->object, b);
    CU_ASSERT_PTR_EQUAL_FATAL(list_pop(list)->object, a);

    list_destroy(list);
}

int setup_list_suite() {
    CU_pSuite suite = CU_add_suite("List tests", NULL, NULL);

    if(NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ADD_TEST(test_list, "list push, pop");

    return 0;
}

/*****************************
 ** Lexer suite             **
 *****************************/

void test_lexer_end() {
    const char *s = "";
    lexer_t *lexer = lexer_init(s, strlen(s));

    CU_ASSERT(lexer != NULL);

    lexer_token_t *token = lexer_next(lexer);

    CU_ASSERT(token != NULL);
    CU_ASSERT(token->type != TOKEN_EOF);
}

void test_lexer_atom() {
    const char *s = "1";
    lexer_t *lexer = lexer_init(s, strlen(s));

    CU_ASSERT(lexer != NULL);

    CU_ASSERT_EQUAL_FATAL((lexer_next(lexer))->type, TOKEN_ATOM);
}

void test_lexer_list() {
    const char *s = "()";
    lexer_t *lexer = lexer_init(s, strlen(s));

    CU_ASSERT(lexer != NULL);

    CU_ASSERT_EQUAL_FATAL((lexer_next(lexer))->type, TOKEN_LIST_START);
    CU_ASSERT_EQUAL_FATAL((lexer_next(lexer))->type, TOKEN_LIST_END);
}

void test_lexer_expect() {
    const char *s = "()";
    lexer_t *lexer = lexer_init(s, strlen(s));

    CU_ASSERT(lexer != NULL);

    CU_ASSERT_PTR_NULL_FATAL(lexer_expect(lexer, TOKEN_LIST_END));
    CU_ASSERT_PTR_NOT_NULL_FATAL(lexer_expect(lexer, TOKEN_LIST_START));
    CU_ASSERT_PTR_NOT_NULL_FATAL(lexer_expect(lexer, TOKEN_LIST_END));
}

int setup_lexer_suite() {
    CU_pSuite suite = CU_add_suite("Lexer tests", NULL, NULL);

    if(NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ADD_TEST(test_lexer_end, "end token");
    ADD_TEST(test_lexer_atom, "atom token");
    ADD_TEST(test_lexer_list, "list token");
    ADD_TEST(test_lexer_expect, "expect token");

    return 0;
}

/*****************************
 ** Lisp suite              **
 *****************************/

static sexpr_t *read(const char *s) {
    sexpr_t *sexpr = lisp_read(s, strlen(s));

    CU_ASSERT_PTR_NOT_NULL_FATAL(sexpr);
    return sexpr;
}

static object_t *eval(const char *s) {
    return lisp_eval(read(s));
}

static const char *print(const char *s) {
    const char *printed = lisp_print(eval(s));

    CU_ASSERT_PTR_NOT_NULL_FATAL(printed);
    return printed;
}

void test_lisp_read_atom() {
    const char *s = "1";
    sexpr_t *sexpr = read(s);

    CU_ASSERT_PTR_NOT_NULL_FATAL(sexpr);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sexpr->object);
    CU_ASSERT_EQUAL_FATAL(sexpr->object->type, OBJECT_INTEGER);
}

void test_lisp_read_list() {
    const char *s = "(a b)";

    read(s);
}

void test_lisp_eval_atom() {
    const char *s = "1";
    object_t *object = eval(s);

    CU_ASSERT_PTR_NOT_NULL_FATAL(object);

    CU_ASSERT_EQUAL_FATAL(object->type, OBJECT_INTEGER);
    CU_ASSERT_EQUAL_FATAL(((object_integer_t *) object)->number, 1);
}

void test_lisp_eval_nil() {
    object_t *o = lisp_eval(read("()"));

    CU_ASSERT_PTR_NULL_FATAL(o);
    CU_ASSERT_STRING_EQUAL_FATAL(lisp_print(o), "NIL");
}

void test_lisp_print_atom_integer() {
    CU_ASSERT_STRING_EQUAL("4", "4");
}

void test_lisp_print_atom_string() {
    const char *s = "\"hello, world!\"";

    ASSERT_PRINT(s, s);
}

void test_lisp_print_list_nil() {
    ASSERT_PRINT("()", "NIL");
}

void test_lisp_symbol_t() {
    ASSERT_PRINT(print("T"), "T");
}

void test_lisp_quote() {
    ASSERT_PRINT("(QUOTE ())", "NIL");
    ASSERT_PRINT("(QUOTE 1)", "1");
    ASSERT_PRINT("(QUOTE A)", "A");
    ASSERT_PRINT("(QUOTE (CONS 1 2))", "(CONS 1 2)");
}

void test_lisp_atom() {
    ASSERT_PRINT("(ATOM (QUOTE A))", "T");
}

void test_lisp_cons() {
    ASSERT_PRINT("(CONS 1 2)", "(1 . 2)");
    ASSERT_PRINT("(CONS 2 (CONS 3 ()))", "(2 3)");
    ASSERT_PRINT("(CONS 3 (CONS 4 5))", "(3 4 . 5)");
    ASSERT_PRINT("(CONS (CONS 4 5) 6)", "((4 . 5) . 6)");

    ASSERT_PRINT("(CONS 1 2)", "(1 . 2)");
    ASSERT_PRINT("(CONS 1 (CONS 2 ()))", "(1 2)");
}

void test_lisp_car() {
    ASSERT_PRINT("(CAR (CONS 3 2))", "3");
    ASSERT_PRINT("(CAR (CONS (CONS 1 2) 3))", "(1 . 2)");
    ASSERT_PRINT("(CAR (CONS 1 (CONS 2 3)))", "1");
}

void test_lisp_cdr() {
    ASSERT_PRINT("(CDR (CONS 3 2))", "2");
    ASSERT_PRINT("(CDR (CONS 1 (CONS 2 3)))", "(2 . 3)");
    ASSERT_PRINT("(CDR (CONS (CONS 1 2) 3))", "3");
}

void test_lisp_eq() {
    ASSERT_PRINT("(EQ (QUOTE A) (QUOTE B))", "NIL");
    ASSERT_PRINT("(EQ (QUOTE A) (QUOTE A))", "T");
    ASSERT_PRINT("(EQ (QUOTE (A B)) (QUOTE (A B)))", "NIL");
    ASSERT_PRINT("(EQ (QUOTE ()) (QUOTE ()))", "T");
    ASSERT_PRINT("(EQ () ())", "T");
    ASSERT_PRINT("(EQ (CONS 1 NIL) (CONS 1 NIL))", "NIL");
}

void test_lisp_cond() {
    ASSERT_PRINT("(COND (T T))", "T");
    ASSERT_PRINT("(COND (NIL T))", "NIL");

    ASSERT_PRINT("(COND ((EQ T T) T))", "T");
    ASSERT_PRINT("(COND ((EQ NIL T) T))", "NIL");
}

int setup_lisp_suite() {
    CU_pSuite suite = CU_add_suite("Lisp tests", NULL, NULL);

    if(NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ADD_TEST(test_lisp_read_atom, "lisp read atom");
    ADD_TEST(test_lisp_read_list, "lisp read list");
    ADD_TEST(test_lisp_eval_atom, "lisp eval atom");
    ADD_TEST(test_lisp_eval_nil, "lisp eval ()");
    ADD_TEST(test_lisp_print_atom_integer, "lisp print atom integer");
    ADD_TEST(test_lisp_print_atom_string, "lisp print atom string");
    ADD_TEST(test_lisp_print_list_nil, "lisp print ()");
    ADD_TEST(test_lisp_symbol_t, "lisp symbol T");
    ADD_TEST(test_lisp_atom, "lisp ATOM");
    ADD_TEST(test_lisp_quote, "lisp QUOTE");
    ADD_TEST(test_lisp_cons, "lisp CONS");
    ADD_TEST(test_lisp_car, "lisp CAR");
    ADD_TEST(test_lisp_cdr, "lisp CDR");
    ADD_TEST(test_lisp_eq, "lisp EQ");
    ADD_TEST(test_lisp_cond, "lisp COND");

    return 0;
}

int main() {
    if(CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    setup_list_suite();
    setup_lexer_suite();
    setup_lisp_suite();

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    CU_cleanup_registry();
    return CU_get_error();
}
