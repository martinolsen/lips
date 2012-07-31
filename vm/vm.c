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

    vm->pc = -1;

    return vm;
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

int vm_encode(vm_op_t op, int dst, int a, int b) {
    return ((op & 0xff) << 24)
        + ((dst & 0xff) << 16)
        + ((a & 0xff) << 8)
        + (b & 0xff);
}

static vm_op_t vm_decode_op(int in) {
    return (in & 0xff000000) >> 24;
}

static vm_op_t vm_decode_dst(int in) {
    return (in & 0x00ff0000) >> 16;
}

static vm_op_t vm_decode_a(int in) {
    return (in & 0x0000ff00) >> 8;
}

static vm_op_t vm_decode_b(int in) {
    return in & 0x000000ff;
}

void vm_run(vm_t *vm) {
    while(1) {
        vm->pc++;

        if(vm->pc > vm->data_sz) {
            fprintf(stderr, "vm_run: pc overflow!\n");
            exit(EXIT_FAILURE);
        }

        int in = vm_peek(vm, vm->pc);

        switch(vm_decode_op(in)) {
            case VM_OP_NOP:
                break;
            case VM_OP_BRK:
                return;
            case VM_OP_MOV:
                vm_reg_set(vm, vm_decode_dst(in), vm_decode_a(in));
                break;
            case VM_OP_ADD:
                vm_reg_set(vm, vm_decode_dst(in),
                        vm_reg_get(vm, vm_decode_a(in))
                        + vm_reg_get(vm, vm_decode_b(in)));
                break;
        }
    }
}

void vm_print(vm_t *vm) {
    printf("VM DUMP:\nRegisters:\n");
    for(size_t i = 0; i < VM_REGS; i++)
        printf(" r%02d: 0x%08x\n", (int) i, vm->regs[i]);

    printf("\nMemory:\n");
    for(size_t i = 0; i < vm->data_sz; i++)
        printf(" 0x%08x: 0x%08x\n", (int) i, vm->data[i]);
}

int vm_as_read_reg(const char *code, size_t *codei) {
    if(code[*codei] != '%' || code[*codei+1] != 'r'
            || code[*codei+2] < '0' || code[*codei+2] > '9') {

        fprintf(stderr, "vm_as_read_reg: expected register at %d (%%r?), not '%c'\n",
                (int) *codei, code[*codei]);
        exit(EXIT_FAILURE);
    }

    *codei += 2;

    int num = code[*codei] - '0';

    *codei += 1;

    return num;
}

vm_t *vm_as(const char *code) {
    size_t codei = 0;
    size_t datai = 0;

    vm_t *vm = vm_new(16);

    while(true) {
        if(code[codei] == 0)
            break;

        while(code[codei] == ' ' || code[codei] == '\n')
            codei++;

        if(code[codei] == 'm' && code[codei+1] == 'o' && code[codei+2] == 'v') {
            codei += 3;

            while(code[codei] == ' ')
                codei++;

            int dst = vm_as_read_reg(code, &codei);

            if(code[codei] != ',') {
                fprintf(stderr, "vm_as: expected ',' at %d\n", (int) codei);
                exit(EXIT_FAILURE);
            }

            codei += 1;

            while(code[codei] == ' ')
                codei++;

            int src = 0;

            while(code[codei] >= '0' && code[codei] <= '9') {
                src += code[codei] - '0';

                if(code[codei+1] >= '0' && code[codei+1] <= '9')
                    src *= 10;

                codei++;
            }

            vm_poke(vm, datai++, vm_encode(VM_OP_MOV, dst, src, 0));
            continue;
        } else if(code[codei] == 'a' && code[codei+1] == 'd' && code[codei+2] == 'd') {
            codei += 3;

            while(code[codei] == ' ')
                codei++;

            int dst = vm_as_read_reg(code, &codei);

            if(code[codei] != ',') {
                fprintf(stderr, "vm_as: expected ',' at %d\n", (int) codei);
                exit(EXIT_FAILURE);
            }

            codei += 1;

            while(code[codei] == ' ')
                codei++;

            int a = vm_as_read_reg(code, &codei);

            if(code[codei] != ',') {
                fprintf(stderr, "vm_as: expected ',' at %d\n", (int) codei);
                exit(EXIT_FAILURE);
            }

            codei += 1;

            while(code[codei] == ' ')
                codei++;

            int b = vm_as_read_reg(code, &codei);

            vm_poke(vm, datai++, vm_encode(VM_OP_ADD, dst, a, b));
            continue;
        } else if(code[codei] == 'b' && code[codei+1] == 'r' && code[codei+2] == 'k') {
            codei += 3;

            vm_poke(vm, datai++, vm_encode(VM_OP_BRK, 0, 0, 0));
            continue;
        }

        printf("vm_as: error at %d (%c)\n", (int) codei, code[codei]);
        exit(EXIT_FAILURE);
    }

    return vm;
}
