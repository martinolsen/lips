#ifndef __VM_H
#define __VM_H

#define VM_REGS 16

typedef enum {
    VM_OP_NOP = 0x00,
    VM_OP_RET = 0x01,
    VM_OP_JMP = 0x04,
    VM_OP_MOV = 0x08,
    VM_OP_ADD = 0x10,
} vm_op_t;

typedef struct {
    int *data;
    size_t data_sz;
    int regs[VM_REGS];
    size_t pc;
} vm_t;

vm_t *vm_new(size_t);

void vm_poke(vm_t *, size_t, int);

int vm_reg_get(vm_t *, size_t);
void vm_reg_set(vm_t *, size_t, int);

int vm_encode(vm_op_t, int, int, int);

void vm_run(vm_t *);

void vm_print(vm_t *);

#endif
