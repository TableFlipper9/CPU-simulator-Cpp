#pragma once
#include <vector>
#include "PipelineRegisters.hpp"
#include "PipelineStages.hpp"
#include "Registerfile.hpp"
#include "Memory.hpp"
#include "Instructions.hpp"
#include "HazardUnit.hpp"

class CPU {
public:
    CPU();

    void loadProgram(const std::vector<Instruction>& program);

    // Reset state while keeping the currently loaded program
    void reset(bool clearMemory = true);

    void tick();

    bool isHalted() const;

    const PipelineRegisters& pipeline() const { return pipe; }
    const std::vector<Instruction>& program() const { return instrMem; }
    const RegisterFile& regFile() const { return regs; }
    const Memory& memory() const { return mem; }

    void dumpRegisters() const;
    void dumpPipeline() const;

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
