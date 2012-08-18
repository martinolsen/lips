#include <CUnit/Basic.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"
#include "vm_as.h"

#define ADD_TEST(s, fn, n) if(NULL == CU_add_test(s, n, fn)) \
        { CU_cleanup_registry(); return CU_get_error(); }

void test_vm() {
    vm_t *vm = vm_new(16);
    CU_ASSERT_PTR_NOT_NULL_FATAL(vm);

    int instructions[] = {
        vm_encode(VM_OP_MOV, 0, 40, 0),
        vm_encode(VM_OP_MOV, 1, 2, 0),
        vm_encode(VM_OP_ADD, 0, 0, 1),
        vm_encode(VM_OP_RET, 0, 0, 0)};

    for(int i = 0; i < 4; i++)
        vm_poke(vm, i, instructions[i]);

    vm_run(vm);

    CU_ASSERT_EQUAL_FATAL(vm_reg_get(vm, 0), 42);
}

void test_as() {
    vm_t *vm = vm_as(
            "mov %r0, 40\n"
            "mov %r1, 2\n"
            "add %r0, %r0, %r1\n"
            "ret");
    CU_ASSERT_PTR_NOT_NULL_FATAL(vm);

    vm_run(vm);

    CU_ASSERT_EQUAL_FATAL(vm_reg_get(vm, 0), 42);
}

void test_vm_ret() {
    vm_t *vm = vm_as("ret\nmov %r0, 42");
    CU_ASSERT_PTR_NOT_NULL_FATAL(vm);

    vm_run(vm);

    CU_ASSERT_NOT_EQUAL_FATAL(vm_reg_get(vm, 0), 42);
}

void test_vm_jmp() {
    vm_t *vm = vm_as(
            "mov %r0, 3\n"
            "jmp %r0\n"
            "mov %r0, 42\n"
            "ret\n");
    CU_ASSERT_PTR_NOT_NULL_FATAL(vm);

    vm_run(vm);

    CU_ASSERT_NOT_EQUAL_FATAL(vm_reg_get(vm, 0), 42);
}

void test_vm_jmp_label() {
    vm_t *vm = vm_as(
            "mov %r0, lbl\n"
            "jmp %r0\n"
            "mov %r0, 42\n"
            "lbl: ret\n");
    CU_ASSERT_PTR_NOT_NULL_FATAL(vm);

    vm_run(vm);

    CU_ASSERT_NOT_EQUAL_FATAL(vm_reg_get(vm, 0), 42);
}

void test_vm_je() {
    vm_t *vm = vm_as(
            "mov %r0, 32\n"
            "mov %r1, 32\n"
            "mov %r2, lbl1\n"
            "je %r2, %r0, %r1\n" // 32 == 32?
            "ret\n"              // return 32 - ERROR
            "lbl1: mov %r0, 42\n"
            "mov %r2, lbl2\n"
            "je %r2, %r0, %r1\n" // 42 == 32?
            "ret\n"              // return 42 - OK
            "lbl2: mov %r0, 52\n"
            "ret\n");            // return 52 - ERROR
    CU_ASSERT_PTR_NOT_NULL_FATAL(vm);

    vm_run(vm);

    CU_ASSERT_EQUAL_FATAL(vm_reg_get(vm, 0), 42);
}

void test_vm_call() {
    vm_t *vm = vm_as(
            "mov %r0, fn\n"
            "call %r0\n"
            "mov %r1, 40\n"
            "add %r0, %r0, %r1\n"
            "ret\n"
            "fn: mov %r0, 2\n"
            "ret\n");
    CU_ASSERT_PTR_NOT_NULL_FATAL(vm);

    vm_run(vm);

    CU_ASSERT_NOT_EQUAL_FATAL(vm_reg_get(vm, 0), 42);
}

int setup_vm_suite() {
    CU_pSuite suite = CU_add_suite("VM", NULL, NULL); \
    if(NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ADD_TEST(suite, test_vm, "Test VM");
    ADD_TEST(suite, test_as, "Test VM assembler");
    ADD_TEST(suite, test_vm_ret, "Test VM - RET");
    ADD_TEST(suite, test_vm_jmp, "Test VM - JMP");
    ADD_TEST(suite, test_vm_jmp_label, "Test VM - JMP w/label");
    ADD_TEST(suite, test_vm_je, "Test VM - JE");
    ADD_TEST(suite, test_vm_call, "Test VM - CALL");

    return 0;
}

int main(void) {
    if(CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    setup_vm_suite();

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    CU_cleanup_registry();
    return CU_get_error();
}
