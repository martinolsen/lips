#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "vm_as.h"
#include "vm.h"

static int vm_as_read_reg(const char *, size_t *);

vm_t *vm_as(const char *code) {
    size_t codei = 0;
    size_t datai = 0;

    vm_t *vm = vm_new(16);

    while(true) {
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
        } else if(code[codei] == 'j' && code[codei+1] == 'm' && code[codei+2] == 'p') {
            code += 3;

            while(code[codei] == ' ')
                codei++;

            int dst = vm_as_read_reg(code, &codei);

            vm_poke(vm, datai++, vm_encode(VM_OP_JMP, dst, 0, 0));
            continue;
        }

        if(code[codei] == 0)
            break;

        printf("vm_as: error at %d (0x%02x/%c)\n",
                (int) codei, code[codei], code[codei]);

        exit(EXIT_FAILURE);
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
