#include <CUnit/Basic.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

void test_vm() {
    vm_t *vm = vm_new(16);
    CU_ASSERT_PTR_NOT_NULL_FATAL(vm);

    int instructions[] = {
        vm_encode(VM_OP_MOV, 0, 40, 0),
        vm_encode(VM_OP_MOV, 1, 2, 0),
        vm_encode(VM_OP_ADD, 0, 0, 1),
        vm_encode(VM_OP_BRK, 0, 0, 0)};

    for(int i = 0; i < 4; i++)
        vm_poke(vm, i, instructions[i]);

    vm_run(vm);

    CU_ASSERT_EQUAL_FATAL(vm_reg_get(vm, 0), 42);
}

void test_as() {
    vm_t *vm = vm_as("mov %r0, 40\nmov %r1, 2\nadd %r0, %r0, %r1\nbrk");
    CU_ASSERT_PTR_NOT_NULL_FATAL(vm);

    vm_run(vm);

    CU_ASSERT_EQUAL_FATAL(vm_reg_get(vm, 0), 42);
}

int setup_vm_suite() {
    CU_pSuite suite = CU_add_suite("VM", NULL, NULL); \
    if(NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "Test VM", test_vm)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if(NULL == CU_add_test(suite, "Test assembler", test_as)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}

int main(int argc __attribute__ ((__unused__)), char **argv __attribute__ ((__unused__))) {
    if(CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    setup_vm_suite();

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    CU_cleanup_registry();
    return CU_get_error();
}
