#pragma once
#include "PipelineRegisters.hpp"
#include "Registerfile.hpp"
#include "Instructions.hpp"
#include "Memory.hpp"
#include "ForwardingUnit.hpp"
#include <vector>

class IFStage {
public:
    void evaluate(PipelineRegisters& pipe,
                  const std::vector<Instruction>& instrMem,
                  int pc_current,
                  int& pc_next,
                  bool stall);
};

class IDStage {
public:
    // When stalled, ID should NOT consume IF/ID, instead it inserts a bubble into ID/EX
    void evaluate(PipelineRegisters& pipe, const RegisterFile& regs, bool stall);
};

class EXStage {
public:
    void evaluate(
        PipelineRegisters& pipe,
        int& pc_next
    );

private:
    ForwardingUnit forwarding;
};


class MEMStage {
public:
    void evaluate(PipelineRegisters& pipe, Memory& mem);
};

class WBStage {
public:
    void evaluate(PipelineRegisters& pipe, RegisterFile& regs);
};