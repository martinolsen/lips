#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "vm_as.h"
#include "vm.h"

static int vm_as_read_reg(const char *, size_t *);
static bool vm_as_expect(const char *, size_t *, const char *);

vm_t *vm_as(const char *code) {
    size_t codei = 0;
    size_t datai = 0;

    vm_t *vm = vm_new(16);

    while(true) {
        while(code[codei] == ' ' || code[codei] == '\n')
            codei++;

        if(code[codei] == 0) {
            break;
        } else if(vm_as_expect(code, &codei, "mov")) {
            while(code[codei] == ' ')
                codei++;

            int dst = vm_as_read_reg(code, &codei);

            if(!vm_as_expect(code, &codei, ",")) {
                fprintf(stderr, "vm_as: expected ',' at %d\n", (int) codei);
                exit(EXIT_FAILURE);
            }

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
        } else if(vm_as_expect(code, &codei, "add")) {
            while(code[codei] == ' ')
                codei++;

            int dst = vm_as_read_reg(code, &codei);

            if(!vm_as_expect(code, &codei, ",")) {
                fprintf(stderr, "vm_as: expected ',' at %d\n", (int) codei);
                exit(EXIT_FAILURE);
            }

            while(code[codei] == ' ')
                codei++;

            int a = vm_as_read_reg(code, &codei);

            if(!vm_as_expect(code, &codei, ",")) {
                fprintf(stderr, "vm_as: expected ',' at %d\n", (int) codei);
                exit(EXIT_FAILURE);
            }

            while(code[codei] == ' ')
                codei++;

            int b = vm_as_read_reg(code, &codei);

            vm_poke(vm, datai++, vm_encode(VM_OP_ADD, dst, a, b));
            continue;
        } else if(vm_as_expect(code, &codei, "brk")) {
            vm_poke(vm, datai++, vm_encode(VM_OP_BRK, 0, 0, 0));
            continue;
        } else if(vm_as_expect(code, &codei, "jmp")) {
            while(code[codei] == ' ')
                codei++;

            int dst = vm_as_read_reg(code, &codei);

            vm_poke(vm, datai++, vm_encode(VM_OP_JMP, dst, 0, 0));
            continue;
        } else {
            printf("vm_as: error at %d (0x%02x/%c)\n",
                    (int) codei, code[codei], code[codei]);

            exit(EXIT_FAILURE);
        }
    }

    return vm;
}

static int vm_as_read_reg(const char *code, size_t *codei) {
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

static bool vm_as_expect(const char *code, size_t *codei, const char *exp) {
    size_t expi = 0, tmpi = *codei;

    while(code[tmpi++] == exp[expi++]) {
        if(exp[expi] == 0) {
            *codei = tmpi;
            return true;
        }
    }

    return false;
}
