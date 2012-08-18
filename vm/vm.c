#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "vm.h"

vm_t *vm_new(size_t data_sz) {
    vm_t *vm = calloc(1, sizeof(vm_t));
    if(vm == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    vm->data_sz = data_sz;
    vm->data = calloc(data_sz, sizeof(int));

    vm->fp = vm_frame_new(NULL, 0);

    return vm;
}

vm_frame_t *vm_frame_new(vm_frame_t *pp, int pc) {
    vm_frame_t *frame = calloc(1, sizeof(vm_frame_t));
    if(frame == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    frame->pc = pc;
    frame->pp = pp;

    return frame;
}

void vm_poke(vm_t *vm, size_t index, int val) {
    if(index > vm->data_sz) {
        fprintf(stderr, "vm_poke: index overflow!\n");
        exit(EXIT_FAILURE);
    }

    vm->data[index] = val;
}

int vm_peek(vm_t *vm, size_t index) {
    if(index > vm->data_sz) {
        fprintf(stderr, "vm_peek: index overflow!\n");
        exit(EXIT_FAILURE);
    }

    return vm->data[index];
}

int vm_reg_get(vm_t *vm, size_t index) {
    if(index > VM_REGS) {
        fprintf(stderr, "vm_reg_get: index overflow!\n");
        exit(EXIT_FAILURE);
    }

    return vm->regs[index];
}

void vm_reg_set(vm_t *vm, size_t index, int val) {
    if(index > VM_REGS) {
        fprintf(stderr, "vm_reg_set: index overflow!\n");
        exit(EXIT_FAILURE);
    }

    vm->regs[index] = val;
}

static void vm_pc_set(vm_t *vm, size_t index) {
    if(vm == NULL || vm->fp == NULL) {
        fprintf(stderr, "vm_pc_set: frame missing!\n");
        exit(EXIT_FAILURE);
    }

    if(index > vm->data_sz) {
        fprintf(stderr, "vm_pc_set: index overflow!\n");
        exit(EXIT_FAILURE);
    }

    vm->fp->pc = index;
}

int vm_encode(vm_op_t op, int dst, int a, int b) {
    return ((op & 0xff) << 24)
        + ((dst & 0xff) << 16)
        + ((a & 0xff) << 8)
        + (b & 0xff);
}

static vm_op_t vm_decode_op(int in) {
    return (in & 0xff000000) >> 24;
}

static vm_op_t vm_decode_a(int in) {
    return (in & 0x00ff0000) >> 16;
}

static vm_op_t vm_decode_b(int in) {
    return (in & 0x0000ff00) >> 8;
}

static vm_op_t vm_decode_c(int in) {
    return in & 0x000000ff;
}

void vm_run(vm_t *vm) {
    while(1) {
        if(vm->fp == NULL) {
            fprintf(stderr, "vm_run: frame missing!\n");
            exit(EXIT_FAILURE);
        }

        if(vm->fp->pc > vm->data_sz) {
            fprintf(stderr, "vm_run: pc overflow!\n");
            exit(EXIT_FAILURE);
        }

        int in = vm_peek(vm, vm->fp->pc++);

        int a = vm_decode_a(in);
        int b = vm_decode_b(in);
        int c = vm_decode_c(in);

        switch(vm_decode_op(in)) {
            case VM_OP_NOP:
                break;
            case VM_OP_CALL:
                vm->fp = vm_frame_new(vm->fp, vm_reg_get(vm, a));
                break;
            case VM_OP_RET:
                return;
            case VM_OP_JMP:
                vm_pc_set(vm, vm_reg_get(vm, a));
                break;
            case VM_OP_JE:
                if(vm_reg_get(vm, b) == vm_reg_get(vm, c))
                    vm_pc_set(vm, vm_reg_get(vm, a));
                break;
            case VM_OP_MOV:
                vm_reg_set(vm, a, b);
                break;
            case VM_OP_ADD:
                vm_reg_set(vm, a, vm_reg_get(vm, b) + vm_reg_get(vm, c));
                break;
        }
    }
}

void vm_print(vm_t *vm) {
    printf("VM DUMP:\nRegisters:\n");
    printf(" pc:  0x%08x\n", (int) vm->fp->pc);
    for(size_t i = 0; i < VM_REGS; i++)
        printf(" r%02d: 0x%08x\n", (int) i, vm_reg_get(vm, i));

    printf("\nMemory:\n");
    for(size_t i = 0; i < vm->data_sz; i++)
        printf(" 0x%08x: 0x%08x\n", (int) i, vm_peek(vm, i));
}
