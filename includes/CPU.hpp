#pragma once
#include <vector>
#include "PipelineRegisters.hpp"
#include "PipelineStages.hpp"
#include "RegisterFile.hpp"
#include "Memory.hpp"
#include "Instructions.hpp"

class CPU {
public:
    CPU();

    void loadProgram(const std::vector<Instruction>& program);

    void tick();

    void dumpRegisters() const;
    void dumpPipeline() const;

    int pc = 0;
    int clock = 0;

private:
    std::vector<Instruction> instrMem;
    PipelineRegisters pipe;

    RegisterFile regs;
    Memory mem;

    IFStage ifStage;
    IDStage idStage;
    EXStage exStage;
    MEMStage memStage;
    WBStage wbStage;
};
