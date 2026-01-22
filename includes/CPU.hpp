#pragma once
#include <vector>
#include "PipelineRegisters.hpp"
#include "PipelineStages.hpp"
#include "RegisterFile.hpp"
#include "Memory.hpp"
#include "Instructions.hpp"
#include "HazardUnit.hpp"

class CPU {
public:
    CPU();

    void loadProgram(const std::vector<Instruction>& program);

    void tick();

    void dumpRegisters() const;
    void dumpPipeline() const;

    // ---- Testing / inspection helpers ----
    // These are intentionally lightweight and keep the core design intact.
    int getReg(int idx) const;
    int getMemWord(int addr) const;
    void setMemWord(int addr, int value);

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

    HazardUnit hazardUnit;
};
