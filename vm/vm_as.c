#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#include "vm_as.h"
#include "vm.h"

#define AST_NODE_ARGS_MAX 3
#define AST_NODE_LABEL_MAX 32

typedef enum {
    AST_NODE_ARG_LABEL = 1,
    AST_NODE_ARG_IMM,
    AST_NODE_ARG_REG
} ast_node_arg_type_t;

typedef struct {
    ast_node_arg_type_t type;
    int reg;
    int imm;
    char *label;
} ast_node_arg_t;

typedef struct {
    char *label;
    vm_op_t op;
    size_t arg_count;
    ast_node_arg_t args[AST_NODE_ARGS_MAX];
} ast_node_t;

typedef struct {
    void *this;
    void *next;
} ll_node_t;

static int vm_as_read_reg(const char *, size_t *);
static bool vm_as_expect(const char *, size_t *, const char *);
static ast_node_t *read_ast_node(const char *, size_t *);
static ll_node_t *read_ast(const char *);
static bool resolve_labels(ll_node_t *);

vm_t *vm_as(const char *code) {
    ll_node_t *ll = read_ast(code);

    if(ll == NULL) {
        fprintf(stderr, "vm_as: error reading AST!\n");
        exit(EXIT_FAILURE);
    }

    if(!resolve_labels(ll)) {
        fprintf(stderr, "vm_as: could not resolve all labels!\n");
        exit(EXIT_FAILURE);
    }

    size_t datai = 0;
    vm_t *vm = vm_new(16);

    while(ll) {
        ast_node_t *node = (ast_node_t*) ll->this;
        int instr;
        switch(node->op) {
            case VM_OP_MOV:
                if(node->arg_count != 2) {
                    fprintf(stderr, "vm_as: 'mov' requires two arguments!\n");
                    exit(EXIT_FAILURE);
                }

                if(node->args[0].type != AST_NODE_ARG_REG ||
                        node->args[1].type != AST_NODE_ARG_IMM) {
                    fprintf(stderr, "vm_as: 'mov' requires reg/imm arguments!\n");
                    exit(EXIT_FAILURE);
                }

                instr = vm_encode(
                        node->op,
                        node->args[0].reg,
                        node->args[1].imm,
                        0);

                break;
            case VM_OP_ADD:
                if(node->arg_count != 3) {
                    fprintf(stderr, "vm_as: 'add' requires three arguments!\n");
                    exit(EXIT_FAILURE);
                }

                if(node->args[0].type != AST_NODE_ARG_REG ||
                        node->args[1].type != AST_NODE_ARG_REG ||
                        node->args[2].type != AST_NODE_ARG_REG) {
                    fprintf(stderr, "vm_as: 'add' requires reg arguments!\n");
                    exit(EXIT_FAILURE);
                }

                instr = vm_encode(
                        node->op,
                        node->args[0].reg,
                        node->args[1].reg,
                        node->args[2].reg);

                break;
            case VM_OP_JMP:
                if(node->arg_count != 1) {
                    fprintf(stderr, "vm_as: 'jmp' requires two arguments!\n");
                    exit(EXIT_FAILURE);
                }

                if(node->args[0].type != AST_NODE_ARG_REG) {
                    fprintf(stderr, "vm_as: 'jmp' requires reg arguments!\n");
                    exit(EXIT_FAILURE);
                }

                instr = vm_encode(node->op, node->args[0].reg, 0, 0);

                break;
            case VM_OP_NOP:
                if(node->arg_count != 0) {
                    fprintf(stderr, "vm_as: 'nop' requires zero arguments!\n");
                    exit(EXIT_FAILURE);
                }

                instr = vm_encode(node->op, 0, 0, 0);

                break;
            case VM_OP_CALL:
                if(node->arg_count != 1) {
                    fprintf(stderr, "vm_as: 'call' requires one argument!\n");
                    exit(EXIT_FAILURE);
                }

                if(node->args[0].type != AST_NODE_ARG_REG) {
                    fprintf(stderr, "vm_as: 'call' requires reg arguments!\n");
                    exit(EXIT_FAILURE);
                }

                instr = vm_encode(node->op, node->args[0].reg, 0, 0);

                break;
            case VM_OP_RET:
                if(node->arg_count != 0) {
                    fprintf(stderr, "vm_as: 'ret' requires zero arguments (%d)!\n", (int) node->arg_count);
                    exit(EXIT_FAILURE);
                }

                instr = vm_encode(node->op, 0, 0, 0);

                break;
        }

        vm_poke(vm, datai++, instr);

        ll = ll->next;
    }

    return vm;
}

static ll_node_t *read_ast(const char *code) {
    size_t codei = 0;

    ll_node_t *first = NULL, *prev = NULL;

    ast_node_t *node;
    while((node = read_ast_node(code, &codei)) != NULL) {
        ll_node_t *next = calloc(1, sizeof(ll_node_t));
        if(next == NULL) {
            perror("calloc");
            exit(EXIT_FAILURE);
        }

        next->this = (void *)node;

        if(first == NULL) {
            first = next;
        } else {
            if(prev == NULL)
                first->next = next;
            else
                prev->next = next;

            prev = next;
        }
    }

    return first;
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

static ast_node_t *read_ast_node(const char *code, size_t *codei) {
    // line      = label-def? ( comment \n )?
    //             op ( ( ( arg',' )? arg',' )? arg )? comment? \n
    // label-def = label':'
    // label     = CHAR+
    // op        = CHAR+
    // arg       = reg | imm | label
    // comment   = ';' .* \n

    while(isspace(code[*codei]) || code[*codei] == '\n')
        (*codei)++;

    ast_node_t *node = calloc(1, sizeof(ast_node_t));
    if(node == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    // Read label
    size_t tmpi = *codei;
    while(isalnum(code[tmpi++])) {
        if(code[tmpi] == ':') {
            char *label = calloc(AST_NODE_LABEL_MAX, sizeof(char));
            if(label == NULL) {
                perror("calloc");
                exit(EXIT_FAILURE);
            }

            size_t labeli = 0;
            while(code[*codei] != ':')
                label[labeli++] = code[(*codei)++];

            (*codei)++;

            while(isspace(code[*codei]) || code[*codei] == '\n')
                (*codei)++;

            node->label = label;

            break;
        }
    }

    if(code[*codei] == 0) {
        free(node);
        return NULL;
    }

    // Read op
    if(vm_as_expect(code, codei, "mov"))
        node->op = VM_OP_MOV;
    else if(vm_as_expect(code, codei, "add"))
        node->op = VM_OP_ADD;
    else if(vm_as_expect(code, codei, "jmp"))
        node->op = VM_OP_JMP;
    else if(vm_as_expect(code, codei, "ret"))
        node->op = VM_OP_RET;
    else if(vm_as_expect(code, codei, "call"))
        node->op = VM_OP_CALL;
    else {
        printf("read_ast_node: error at %d (0x%02x/%c)\n",
                (int) *codei, code[*codei], code[*codei]);

        exit(EXIT_FAILURE);
    }

    // Read args
    do {
        if(code[*codei] == ',') {
            if(node->arg_count == 0) {
                fprintf(stderr, "read_ast_node: unexpected ',' at %d\n",
                        (int) *codei);
                exit(EXIT_FAILURE);
            }

            (*codei)++;
        }

        if(node->arg_count > AST_NODE_ARGS_MAX) {
            fprintf(stderr, "read_ast_node: too many arguments at %d\n",
                    (int) *codei);
            exit(EXIT_FAILURE);
        }

        while(isblank(code[*codei]))
            (*codei)++;

        if(code[*codei] == 0 || code[*codei] == '\n')
            break;

        if(code[*codei] == '%') { // read reg
            node->args[node->arg_count].type = AST_NODE_ARG_REG;
            node->args[node->arg_count].reg = vm_as_read_reg(code, codei);
        } else if(isdigit(code[*codei])) { // read imm
            node->args[node->arg_count].type = AST_NODE_ARG_IMM;

            int arg = 0;
            while(isdigit(code[*codei])) {
                arg += code[*codei] - '0';

                (*codei)++;

                if(isdigit(code[*codei]))
                    arg *= 10;
            }

            node->args[node->arg_count].imm = arg;
        } else if(isalnum(code[*codei])) { // read label
            node->args[node->arg_count].type = AST_NODE_ARG_LABEL;
            node->args[node->arg_count].label =
                calloc(AST_NODE_LABEL_MAX + 1, sizeof(char));

            size_t argi = 0;
            while(isalnum(code[*codei])) {
                if(argi >= AST_NODE_LABEL_MAX) {
                    fprintf(stderr, "read_ast_node: label overflow at %d\n",
                            (int) *codei);
                    exit(EXIT_FAILURE);
                }

                node->args[node->arg_count].label[argi++] = code[(*codei)++];
            }
        } else {
            fprintf(stderr, "read_ast_node: unknown argument type at %d (%c)\n",
                (int) *codei, code[*codei]);

            exit(EXIT_FAILURE);
        }

        node->arg_count++;
    } while(code[*codei] == ',' && node->arg_count < AST_NODE_ARGS_MAX);

    // Read comment / EOL

    return node;
}

static bool resolve_labels(ll_node_t *first) {
    ll_node_t *this = first;

    while(this != NULL) {
        ast_node_t *node = (ast_node_t *)this->this;

        for(int i = 0; i < AST_NODE_ARGS_MAX; i++) {
            if(node->args[i].type != AST_NODE_ARG_LABEL)
                continue;

            int pos = 0;
            ll_node_t *tmp_this = first;
            while(tmp_this) {
                ast_node_t *tmp_node = (ast_node_t *)tmp_this->this;

                if(tmp_node->label != NULL) {
                    if(strcmp(node->args[i].label, tmp_node->label) == 0) {
                        node->args[i].type = AST_NODE_ARG_IMM;
                        node->args[i].imm = pos;

                        break;
                    }
                }

                pos++;
                tmp_this = tmp_this->next;
            }

            if(node->args[i].type == AST_NODE_ARG_LABEL) {
                fprintf(stderr, "unresolved label: %s\n", node->args[i].label);
                exit(EXIT_FAILURE);
            }
        }

        this = this->next;
    }

    return true;
}
