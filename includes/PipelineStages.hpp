#pragma once
#include "PipelineRegisters.hpp"
#include "RegisterFile.hpp"
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
    void evaluate(PipelineRegisters& pipe, const RegisterFile& regs, bool& stall);
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