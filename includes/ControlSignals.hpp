#pragma once

enum class ALUOp {
    NONE,
    ADD,
    SUB,
    AND,
    OR,
    XOR,
    SLT
};

enum class BranchType {
    NONE,
    BEQ,
    BNE
};

enum class JumpType {
    NONE,
    J,
    JR,
    JAL
};

struct ControlSignals {
    bool regWrite = false;
    bool memRead  = false;
    bool memWrite = false;
    bool memToReg = false;
    bool aluSrcImm = false;

    ALUOp aluOp = ALUOp::NONE;

    BranchType branch = BranchType::NONE;
    JumpType   jump   = JumpType::NONE;

    int destReg = -1;
};
