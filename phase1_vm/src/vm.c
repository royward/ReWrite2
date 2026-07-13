#include "vm.h"
#include <stdlib.h>
#include <inttypes.h>

#define STACK_SIZE 1000000
#define REGISTERS_SIZE 1000
#define LABEL_COUNT 1000

#define OP_LABEL 0x01
#define OP_MIN_BRANCH 0xF0

int program_link(Program* program) {
    uint32_t* labels=(uint32_t*)malloc(program->label_count*sizeof(uint32_t));
    if(!labels) {
        fprintf(stderr, "fatal: problem allocating memory\n");
        return EXIT_FAILURE;
    }
    // first get the label offsets
    for(uint32_t i=0;i<program->count;i++) {
        Operation* operation=&program->code[i];
        if(operation->op==OP_LABEL) {
            labels[operation->dst]=i;
        }
    }
    // now relabel
    for(uint32_t i=0;i<program->count;i++) {
        Operation* operation=&program->code[i];
        if(operation->op>=OP_MIN_BRANCH) {
            operation->dst=labels[operation->dst];
        }
    }
    program->labels=labels;
    return EXIT_SUCCESS;
}

int program_load(Program* program, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if(!f) {
        fprintf(stderr, "fatal: problem loading file\n");
        return EXIT_FAILURE;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if(size%sizeof(Operation)!=0) {
        fprintf(stderr, "fatal: problem loading file - incomplete\n");
        fclose(f);
        return EXIT_FAILURE;
    }
    program->code = malloc(size);
    if(!program->code) {
        fprintf(stderr, "fatal: problem allocating memory\n");
        fclose(f);
        return EXIT_FAILURE;
    }
    fread(program->code, 1, size, f);
    fclose(f);
    program->count = size / sizeof(Operation);
    program->label_count = LABEL_COUNT;
    int ok=program_link(program);
    if(ok) {
        free(program->code);
        fprintf(stderr, "fatal: problem allocating memory\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void program_unload(Program* program) {
    free(program->code);
    free(program->labels);
}

int execution_init(ExecutionState* exe, Program* p) {
    (void)p;
    exe->registers = (uint64_t*)malloc(REGISTERS_SIZE*sizeof(uint64_t));
    exe->stack = (uint8_t*)malloc(STACK_SIZE*sizeof(uint8_t));
    if(!exe->registers || !exe->stack) {
        free(exe->registers);
        free(exe->stack);
        fprintf(stderr, "fatal: problem allocating memory\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void execution_unload(ExecutionState* exe) {
    free(exe->registers);
    free(exe->stack);
}

#define OP_LABEL 0x01
#define OP_ERROR 0x02
#define OP_RET 0x03
#define OP_MOVE 0x10
#define OP_CMP_NE 0xF0
#define OP_CMP_EQ 0xF8

#define BIND_IMM 0
#define BIND_REG 1
#define BIND_STACK 2

void program_display_label(Program* program, FILE* out, Operation* operation) {
    uint32_t label=0;
    for(uint32_t i=0;i<program->label_count;i++) {
        if(program->labels[i]==operation->dst) {
            label=i;
            break;
        }
    }
    fprintf(out,"%d @line %d",label,operation->dst);
}

void display_operand(FILE* out, uint8_t flag, int64_t val) {
    switch(flag) {
        case BIND_IMM: {
            fprintf(out,"%" PRId64,val);
        } break;
        case BIND_REG: {
            fprintf(out,"r%" PRIu64,val);
        } break;
        case BIND_STACK: {
            fprintf(out,"sp+%" PRIu64,val);
        } break;
    }
}

void program_disassemble(Program* program, FILE* out) {
    fprintf(out,"  line func:rule op\n");
    for(uint32_t i=0;i<program->count;i++) {
        Operation* operation=&program->code[i];
        fprintf(out,"%6d %4d %3d: %02x: ",i,operation->fn_id,operation->rule_id,operation->op);
        uint8_t op=operation->op;
        switch(op) {
            case OP_LABEL: {
                fprintf(out,"label %d",operation->dst);
            } break;
            case OP_ERROR: {
                fprintf(out,"error");
            } break;
            case OP_RET: {
                fprintf(out,"ret");
            } break;
            case OP_MOVE: case OP_MOVE+1: case OP_MOVE+2: case OP_MOVE+3: case OP_MOVE+4: {
                uint32_t sz=1<<(op&7);
                fprintf(out,"move%d ",sz);
                display_operand(out,operation->flags_dst,operation->dst);
                fprintf(out," <- ");
                display_operand(out,operation->flags_src1,operation->src1);
            } break;
            case OP_CMP_NE: case OP_CMP_NE+1: case OP_CMP_NE+2: case OP_CMP_NE+3: case OP_CMP_NE+4: {
                uint32_t sz=1<<(op&7);
                fprintf(out,"test%d ",sz);
                display_operand(out,operation->flags_src1,operation->src1);
                fprintf(out," != ");
                display_operand(out,operation->flags_src2,operation->src2);
                fprintf(out," goto ");
                program_display_label(program,out,operation);
            } break;
            case OP_CMP_EQ: case OP_CMP_EQ+1: case OP_CMP_EQ+2: case OP_CMP_EQ+3: case OP_CMP_EQ+4: {
                uint32_t sz=1<<(op&7);
                fprintf(out,"test%d ",sz);
                display_operand(out,operation->flags_src1,operation->src1);
                fprintf(out," == ");
                display_operand(out,operation->flags_src2,operation->src2);
                fprintf(out," goto ");
                program_display_label(program,out,operation);
            } break;
            default: fprintf(out,"unknown");
        }
        fprintf(out,"\n");
    }
}

void operand_store(ExecutionState* exe, Operation* operation, uint64_t value, uint32_t sz) {
    switch(operation->flags_dst) {
        case BIND_IMM: {
            fprintf(stderr,"Fatal error trying to store to an immediate\n");
            exit(EXIT_FAILURE);
        } break;
        case BIND_REG: {
            value&=(sz>=8)?(uint64_t)-1LL:(uint64_t)((1LL<<(sz<<3))-1);
            exe->registers[operation->dst]=value;
        } break;
        case BIND_STACK: {
            uint8_t* addr=&exe->stack[operation->dst];
            switch(sz) { // alignment is guaranteed by the compiler
                case 1:*((uint8_t*)addr)=(uint8_t)value; break;
                case 2:*((uint16_t*)addr)=(uint16_t)value; break;
                case 4:*((uint32_t*)addr)=(uint32_t)value; break;
                case 8:*((uint64_t*)addr)=(uint64_t)value; break;
                default: fprintf(stderr,"unknown size in operand_store\n"); exit(EXIT_FAILURE);
            }
        } break;
        default: {
            fprintf(stderr,"unknown flag in operand_store\n");
            exit(EXIT_FAILURE);
        }
    }
}

uint64_t operand_load(ExecutionState* exe, uint32_t sz, uint32_t flags_src, uint64_t src) {
    uint64_t mask=(sz>=8)?(uint64_t)-1LL:(uint64_t)((1LL<<(sz<<3))-1);
    switch(flags_src) {
        case BIND_IMM: {
            return src&mask;
        } break;
        case BIND_REG: {
            return exe->registers[src]&mask;
        } break;
        case BIND_STACK: {
            uint8_t* addr=&exe->stack[src];
            uint64_t value;
            switch(sz) { // alignment is guaranteed by the compiler
                case 1:value=*((uint8_t*)addr); break;
                case 2:value=*((uint16_t*)addr); break;
                case 4:value=*((uint32_t*)addr); break;
                case 8:value=*((uint64_t*)addr); break;
                default: fprintf(stderr,"unknown size in operand_load\n"); exit(EXIT_FAILURE);
            }
            return value&mask;
        } break;
        default: {
            fprintf(stderr,"unknown flag in operand_load\n");
            exit(EXIT_FAILURE);
        }
    }
}

int program_execute(Program* program, ExecutionState* exe, uint32_t in_pc) {
    uint32_t sp=0; // make sure we are start, even if it was run before
    uint32_t pc=in_pc;
    while(true) {
        Operation* operation=&program->code[pc];
        uint8_t op=operation->op;
        switch(op) {
            case OP_LABEL: {
                // NOP
            } break;
            case OP_ERROR: {
                return EXIT_FAILURE;
            };
            case OP_RET: {
                if(sp==0) {
                    return EXIT_SUCCESS;
                } else {
                    // TODO
                }
            } break;
            case OP_MOVE: case OP_MOVE+1: case OP_MOVE+2: case OP_MOVE+3: case OP_MOVE+4: {
                uint32_t sz = 1 << (op & 7);
                uint64_t val = operand_load(exe, sz, operation->flags_src1, operation->src1);
                operand_store(exe, operation, val, sz);
            } break;
            case OP_CMP_NE: case OP_CMP_NE+1: case OP_CMP_NE+2: case OP_CMP_NE+3: case OP_CMP_NE+4: {
                uint32_t sz = 1 << (op & 7);
                uint64_t src1 = operand_load(exe, sz, operation->flags_src1, operation->src1);
                uint64_t src2 = operand_load(exe, sz, operation->flags_src2, operation->src2);
                if(src1 != src2) {
                    pc = operation->dst-1; // -1 because pc++ at end of loop
                }
            } break;
            case OP_CMP_EQ: case OP_CMP_EQ+1: case OP_CMP_EQ+2: case OP_CMP_EQ+3: case OP_CMP_EQ+4: {
                uint32_t sz = 1 << (op & 7);
                uint64_t src1 = operand_load(exe, sz, operation->flags_src1, operation->src1);
                uint64_t src2 = operand_load(exe, sz, operation->flags_src2, operation->src2);
                if(src1 == src2) {
                    pc = operation->dst-1; // -1 because pc++ at end of loop
                }
            } break;
            default: {
                fprintf(stderr,"Illegal Instruction\n");
                exit(EXIT_FAILURE);
            }
        }
        pc++;
    }
}


