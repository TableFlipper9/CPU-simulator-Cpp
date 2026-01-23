#pragma once
#include <string>

enum class Opcode {
    NOP,

    // R-type
    ADD,
    SUB,
    AND,
    OR,
    SLT,
    JR,

    // I-type
    ADDI,
    ANDI,
    ORI,
    LW,
    SW,
    BEQ,
    BNE,

    // J-type
    J,
    JAL
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
