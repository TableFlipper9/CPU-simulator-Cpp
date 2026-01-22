#include "CPU.hpp"
#include <iostream>

CPU::CPU()
: instrMem()
, mem(1024)
{
    pc = 0;
    clock = 0;
}

void CPU::loadProgram(const std::vector<Instruction>& program) {
    instrMem = program;
    pc = 0;
    clock = 0;

    // clear pipeline
    pipe.if_id = IF_ID{};
    pipe.id_ex = ID_EX{};
    pipe.ex_mem = EX_MEM{};
    pipe.mem_wb = MEM_WB{};
    pipe.clearNext();
}

void CPU::tick() {
    int pc_next = pc;

    // Detect hazards based on the *current* pipeline state.
    const HazardResult hz = hazardUnit.detect(pipe.if_id, pipe.id_ex);
    const bool stall = hz.stall;

    pipe.clearNext();

    // IF/ID are the only stages that stall on a load-use hazard.
    ifStage.evaluate(pipe, instrMem, pc, pc_next, stall);
    idStage.evaluate(pipe, regs, stall);

    // Model same-cycle forwarding of load data from MEM->EX by evaluating MEM
    // before EX. All stages still read the *current* pipeline registers and
    // write only to *_next.
    memStage.evaluate(pipe, mem);
    exStage.evaluate(pipe, pc_next);
    wbStage.evaluate(pipe, regs);

    pipe.if_id = pipe.if_id_next;
    pipe.id_ex = pipe.id_ex_next;
    pipe.ex_mem = pipe.ex_mem_next;
    pipe.mem_wb = pipe.mem_wb_next;

    regs.commit();
    mem.commit();

    pc = pc_next;
    clock++;
}

void CPU::dumpRegisters() const {
    const auto& r = regs.getRegs();
    std::cout << "Registers:\n";
    for (int i=0;i<32;i++) {
        std::cout << " $" << i << ": " << r[i];
        if ((i%8)==7) std::cout << "\n";
        else std::cout << "\t";
    }
    std::cout << std::flush;
}

void CPU::dumpPipeline() const {
    auto dumpIF = [&]() {
        if (!pipe.if_id.valid) { std::cout << "IF: <empty>\n"; return; }
        std::cout << "IF: pc=" << pipe.if_id.pc << " op=" << (int)pipe.if_id.rawInstr.op << " txt=" << pipe.if_id.rawInstr.raw_text << "\n";
    };
    auto dumpID = [&]() {
        if (!pipe.id_ex.valid) { std::cout << "ID/EX: <empty>\n"; return; }
        std::cout << "ID/EX: pc=" << pipe.id_ex.pc << " rs=" << pipe.id_ex.rs << " rt=" << pipe.id_ex.rt << " imm=" << pipe.id_ex.imm << "\n";
    };
    auto dumpEX = [&]() {
        if (!pipe.ex_mem.valid) { std::cout << "EX/MEM: <empty>\n"; return; }
        std::cout << "EX/MEM: alu=" << pipe.ex_mem.alu_result << " zero=" << pipe.ex_mem.zero << "\n";
    };
    auto dumpMEM = [&]() {
        if (!pipe.mem_wb.valid) { std::cout << "MEM/WB: <empty>\n"; return; }
        std::cout << "MEM/WB: alu=" << pipe.mem_wb.alu_result << " mem=" << pipe.mem_wb.mem_data << "\n";
    };

    std::cout << "Clock: " << clock << " PC: " << pc << "\n";
    dumpIF();
    dumpID();
    dumpEX();
    dumpMEM();
    std::cout << std::flush;
}

int CPU::getReg(int idx) const {
    return regs.read(idx);
}

int CPU::getMemWord(int addr) const {
    return mem.read(addr);
}

void CPU::setMemWord(int addr, int value) {
    // For tests/initialization we want an immediate, deterministic effect.
    mem.writeNext(addr, value);
    mem.commit();
}
