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

    // Common fields for a simplified instruction representation.
    int rs = 0;
    int rt = 0;
    int rd = 0;

    // Immediate for I-type, already sign/zero-extended by the creator of Instruction (for now).
    int imm = 0;

    // Absolute jump target (interpreted as an instruction index in this simulator).
    int addr = 0;

    std::string raw_text;
};
