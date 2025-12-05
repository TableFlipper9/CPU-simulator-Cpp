#pragma once

enum class ALUOp {
    NONE,
    ADD,
    SUB
};

struct ControlSignals {
    bool regWrite = false;
    bool memRead  = false;
    bool memWrite = false;
    bool memToReg = false; 
    bool branch   = false;

    ALUOp aluOp = ALUOp::NONE;

    int destReg = -1;
};
