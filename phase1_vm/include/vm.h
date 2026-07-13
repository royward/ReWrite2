#include <stdint.h>
#include <stdbool.h>  // for bool, true, false (C99)
#include <stdio.h>

typedef struct {
// 0
    uint8_t op;
    uint8_t type;
    uint16_t fn_id;
// 4
    uint8_t rule_id;
    uint8_t flags_dst;
    uint8_t flags_src1;
    uint8_t flags_src2;
// 8
    uint32_t dst;
    uint32_t reserved1;  // available for future use
// 16
    uint64_t src1;
// 24
    uint64_t src2;
// 32
} Operation;

typedef struct {
    Operation* code;
    uint32_t count;
    uint32_t label_count;
    uint32_t* labels;
} Program;

typedef struct {
    uint32_t sp;
    uint64_t* registers;
    uint8_t* stack;
} ExecutionState;

int program_load(Program* program, const char* filename);
void program_unload(Program* program);
void program_disassemble(Program* program, FILE* out);
int program_execute(Program* program, ExecutionState* exe, uint32_t pc);
int execution_init(ExecutionState* exe, Program* p);
void execution_unload(ExecutionState* exe);

