#pragma once
#include <string>

enum class Opcode {
    NOP,
    ADD,
    SUB,
    ADDI,
    LW,
    SW,
    BEQ,
    J
};

struct Instruction {
    Opcode op = Opcode::NOP;
    int rs = 0;
    int rt = 0;
    int rd = 0;
    int imm = 0; 
    int addr = 0; 
    std::string raw_text; 
};
