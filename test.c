#include <CUnit/Basic.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "lisp.h"
#include "list.h"

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

    if(NULL == CU_add_test(suite, "list push, pop", test_list)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}

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

    if(NULL == CU_add_test(suite, "end token", test_lexer_end)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "atom token", test_lexer_atom)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "list token", test_lexer_list)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "expect token", test_lexer_expect)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}

void test_lisp_read_atom() {
    const char *s = "1";
    lisp_t *lisp = lisp_new();

    sexpr_t *sexpr = lisp_read(lisp, s, strlen(s));

    CU_ASSERT_PTR_NOT_NULL_FATAL(sexpr);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sexpr->object);
    CU_ASSERT_EQUAL_FATAL(sexpr->object->type, OBJECT_ATOM);

    lisp_destroy(lisp);
}

void test_lisp_read_list() {
    const char *s = "(1 2)";
    lisp_t *lisp = lisp_new();

    sexpr_t *sexpr = lisp_read(lisp, s, strlen(s));

    CU_ASSERT_PTR_NOT_NULL_FATAL(sexpr);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sexpr->object);
    CU_ASSERT_EQUAL_FATAL(sexpr->object->type, OBJECT_LIST);

    lisp_destroy(lisp);
}

void test_lisp_eval_atom() {
    const char *s = "1";
    lisp_t *lisp = lisp_new();

    sexpr_t *sexpr = lisp_read(lisp, s, strlen(s));

    CU_ASSERT_PTR_NOT_NULL_FATAL(sexpr);

    object_t *object = lisp_eval(lisp, sexpr);

    CU_ASSERT_PTR_NOT_NULL_FATAL(object);

    CU_ASSERT_EQUAL_FATAL(object->type, OBJECT_ATOM);
    CU_ASSERT_PTR_NOT_NULL_FATAL(((object_atom_t *) object)->atom);
    CU_ASSERT_EQUAL_FATAL(((object_atom_t *) object)->atom->type,
                          ATOM_INTEGER);

    lisp_destroy(lisp);
}

void test_lisp_eval_list() {
    const char *s = "(1 2)";
    lisp_t *lisp = lisp_new();

    sexpr_t *sexpr = lisp_read(lisp, s, strlen(s));

    CU_ASSERT_PTR_NOT_NULL_FATAL(sexpr);

    object_t *object = lisp_eval(lisp, sexpr);

    CU_ASSERT_PTR_NOT_NULL_FATAL(object);

    CU_ASSERT_EQUAL(object->type, OBJECT_LIST);

    lisp_destroy(lisp);
}

void test_lisp_print_atom() {
    const char *s = "4";
    lisp_t *lisp = lisp_new();

    sexpr_t *sexpr = lisp_read(lisp, s, strlen(s));

    CU_ASSERT_PTR_NOT_NULL_FATAL(sexpr);

    object_t *object = lisp_eval(lisp, sexpr);

    CU_ASSERT_PTR_NOT_NULL_FATAL(object);

    const char *printed = lisp_print(lisp, object);

    CU_ASSERT_PTR_NOT_NULL_FATAL(printed);
    CU_ASSERT_STRING_EQUAL(printed, s);

    lisp_destroy(lisp);
}

void test_lisp_print_atom_string() {
    const char *s = "\"hello, world!\"";
    lisp_t *lisp = lisp_new();

    sexpr_t *sexpr = lisp_read(lisp, s, strlen(s));

    CU_ASSERT_PTR_NOT_NULL_FATAL(sexpr);

    object_t *object = lisp_eval(lisp, sexpr);

    CU_ASSERT_PTR_NOT_NULL_FATAL(object);

    const char *printed = lisp_print(lisp, object);

    CU_ASSERT_PTR_NOT_NULL_FATAL(printed);
    CU_ASSERT_STRING_EQUAL_FATAL(printed, s);

    lisp_destroy(lisp);
}

void test_lisp_print_list() {
    const char *s = "(1 2)";
    lisp_t *lisp = lisp_new();

    sexpr_t *sexpr = lisp_read(lisp, s, strlen(s));

    CU_ASSERT_PTR_NOT_NULL_FATAL(sexpr);

    object_t *object = lisp_eval(lisp, sexpr);

    CU_ASSERT_PTR_NOT_NULL_FATAL(object);

    const char *printed = lisp_print(lisp, object);

    CU_ASSERT_PTR_NOT_NULL_FATAL(printed);
    CU_ASSERT_STRING_EQUAL(printed, s);

    lisp_destroy(lisp);
}

object_t *eval(lisp_t * lisp, const char *s) {
    return lisp_eval(lisp, lisp_read(lisp, s, strlen(s)));
}

void test_lisp_fun() {
    lisp_t *lisp = lisp_new();

    CU_ASSERT_PTR_NOT_NULL_FATAL(eval(lisp, "(defun x () (5))"));

    object_t *object = eval(lisp, "(x)");

    CU_ASSERT_PTR_NOT_NULL_FATAL(object);
    CU_ASSERT_EQUAL_FATAL(object->type, OBJECT_ATOM);
    CU_ASSERT_PTR_NOT_NULL_FATAL(((object_atom_t *) object)->atom);
    CU_ASSERT_EQUAL_FATAL((((object_atom_t *) object)->atom)->type,
                          ATOM_INTEGER);
    CU_ASSERT_EQUAL_FATAL(((atom_integer_t *) ((object_atom_t *) object)->
                           atom)->number, 5);

    lisp_destroy(lisp);
}

int setup_lisp_suite() {
    CU_pSuite suite = CU_add_suite("Lisp tests", NULL, NULL);

    if(NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "lisp read atom", test_lisp_read_atom)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "lisp read list", test_lisp_read_list)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "lisp eval atom", test_lisp_eval_atom)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "lisp eval list", test_lisp_eval_list)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "lisp print atom", test_lisp_print_atom)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL ==
       CU_add_test(suite, "lisp print atom string",
                   test_lisp_print_atom_string)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "lisp print list", test_lisp_print_list)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "lisp function", test_lisp_fun)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

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
