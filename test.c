#include <CUnit/Basic.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "lisp.h"
#include "list.h"

#define ADD_TEST(t, s) do { if(NULL == CU_add_test(suite, s, t)) { \
        CU_cleanup_registry(); return CU_get_error(); } } while(0);

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
    object_t *object = lisp_eval(read(s));

    CU_ASSERT_PTR_NOT_NULL_FATAL(object);
    return object;
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
    CU_ASSERT_EQUAL_FATAL(sexpr->object->type, OBJECT_ATOM);
}

void test_lisp_read_list() {
    const char *s = "(a b)";

    read(s);
}

void test_lisp_eval_atom() {
    const char *s = "1";
    object_t *object = eval(s);

    CU_ASSERT_PTR_NOT_NULL_FATAL(object);

    CU_ASSERT_EQUAL_FATAL(object->type, OBJECT_ATOM);
    CU_ASSERT_PTR_NOT_NULL_FATAL(((object_atom_t *) object)->atom);
    CU_ASSERT_EQUAL_FATAL(((object_atom_t *) object)->atom->type,
                          ATOM_INTEGER);
}

void test_lisp_eval_list() {
    object_t *object = eval("()");

    CU_ASSERT_PTR_NOT_NULL_FATAL(object);
}

void test_lisp_print_atom_integer() {
    const char *s = "4";
    const char *printed = print(s);

    CU_ASSERT_STRING_EQUAL(printed, s);
}

void test_lisp_print_atom_string() {
    const char *s = "\"hello, world!\"";
    const char *printed = print(s);

    CU_ASSERT_STRING_EQUAL_FATAL(printed, s);
}

void test_lisp_print_list_nil() {
    const char *printed = print("()");

    CU_ASSERT_STRING_EQUAL_FATAL(printed, "NIL");
}

void test_lisp_cons() {
    CU_ASSERT_STRING_EQUAL_FATAL(print("(CONS 1 2)"), "(1 . 2)");
    CU_ASSERT_STRING_EQUAL_FATAL(print("(CONS 2 (CONS 3 ()))"), "(2 3)");
    CU_ASSERT_STRING_EQUAL_FATAL(print("(CONS 3 (CONS 4 5))"), "(3 4 . 5)");
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
    ADD_TEST(test_lisp_eval_list, "lisp eval list");
    ADD_TEST(test_lisp_print_atom_integer, "lisp print atom integer");
    ADD_TEST(test_lisp_print_atom_string, "lisp print atom string");
    ADD_TEST(test_lisp_print_list_nil, "lisp print ()");
    ADD_TEST(test_lisp_cons, "lisp CONS");

    return 0;
}

/*****************************
 ** Common Lisp suite       **
 *****************************/

void test_clisp_list() {
    const char *s = "(list 1 2)";

    CU_ASSERT_STRING_EQUAL(print(s), "(1 2)");
}

void test_clisp_defun() {
    CU_ASSERT_PTR_NOT_NULL_FATAL(eval("(defun x () (5))"));

    object_t *object = eval("(x)");

    CU_ASSERT_PTR_NOT_NULL_FATAL(object);
    CU_ASSERT_EQUAL_FATAL(object->type, OBJECT_ATOM);
    CU_ASSERT_PTR_NOT_NULL_FATAL(((object_atom_t *) object)->atom);
    CU_ASSERT_EQUAL_FATAL((((object_atom_t *) object)->atom)->type,
                          ATOM_INTEGER);
    CU_ASSERT_EQUAL_FATAL(((atom_integer_t *) ((object_atom_t *)
                                               object)->atom)->number, 5);
}

int setup_clisp_suite() {
    CU_pSuite suite = CU_add_suite("Common Lisp tests", NULL, NULL);

    if(NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ADD_TEST(test_clisp_list, "lisp LIST");
    ADD_TEST(test_clisp_defun, "lisp DEFUN");

    return 0;
}

int main() {
    if(CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    setup_list_suite();
    setup_lexer_suite();
    setup_lisp_suite();
    setup_clisp_suite();

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    CU_cleanup_registry();
    return CU_get_error();
}
