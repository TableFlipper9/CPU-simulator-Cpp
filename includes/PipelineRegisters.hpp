#pragma once
#include "Instructions.hpp"
#include "ControlSignals.hpp"

struct IF_ID {
    Instruction rawInstr; 
    int pc = 0;
    bool valid = false;
};

struct ID_EX {
    int pc = 0;
    int val_rs = 0;
    int val_rt = 0;
    int imm = 0;
    int addr = 0;
    int rs = 0;
    int rt = 0;

    ControlSignals ctrl;

    bool valid = false;
};

struct EX_MEM {
    int alu_result = 0;
    int val_rt = 0; 
    int branchTarget = 0;
    bool zero = false;

    ControlSignals ctrl;

    bool valid = false;
};

struct MEM_WB {
    int alu_result = 0;
    int mem_data = 0;

    ControlSignals ctrl;

    bool valid = false;
};

struct PipelineRegisters {
    IF_ID if_id, if_id_next;
    ID_EX id_ex, id_ex_next;
    EX_MEM ex_mem, ex_mem_next;
    MEM_WB mem_wb, mem_wb_next;

    void clearNext();
};
