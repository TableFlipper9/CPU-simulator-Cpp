#include "PipelineStages.hpp"

void IFStage::evaluate(
    PipelineRegisters& pipe,
    const std::vector<Instruction>& instrMem,
    int pc_current,
    int& pc_next,
    bool stall
) {
    if (stall) {
        pipe.if_id_next = pipe.if_id;
        pc_next = pc_current;
        return;
    }

    if (pc_current < 0 || pc_current >= (int)instrMem.size()) {
        pipe.if_id_next.valid = false;
        pc_next = pc_current;
        return;
    }

    pipe.if_id_next.rawInstr = instrMem[pc_current];
    pipe.if_id_next.pc = pc_current;
    pipe.if_id_next.valid = true;

    pc_next = pc_current + 1;
}


void IDStage::evaluate(PipelineRegisters& pipe, const RegisterFile& regs, bool& stall) {
    const IF_ID& in = pipe.if_id;

    if (!in.valid) {
        pipe.id_ex_next.valid = false;
        return;
    }

    const Instruction& di = in.rawInstr;
    ID_EX& out = pipe.id_ex_next;

    out.pc = in.pc;
    out.rs = di.rs;
    out.rt = di.rt;
    out.imm = di.imm;
    out.val_rs = regs.read(di.rs);
    out.val_rt = regs.read(di.rt);

    ControlSignals c;
    c.aluOp = ALUOp::NONE;
    c.destReg = -1;

    switch (di.op) {
        case Opcode::ADD:
            c.regWrite = true;
            c.aluOp = ALUOp::ADD;
            c.destReg = di.rd;
            break;
        case Opcode::SUB:
            c.regWrite = true;
            c.aluOp = ALUOp::SUB;
            c.destReg = di.rd;
            break;
        case Opcode::ADDI:
            c.regWrite = true;
            c.aluOp = ALUOp::ADD;
            c.aluSrcImm = true;
            c.destReg = di.rt;
            break;
        case Opcode::LW:
            c.regWrite = true;
            c.memRead = true;
            c.memToReg = true;
            c.aluOp = ALUOp::ADD;
            c.aluSrcImm = true;
            c.destReg = di.rt;
            break;
        case Opcode::SW:
            c.memWrite = true;
            c.aluOp = ALUOp::ADD;
            c.aluSrcImm = true;
            break;
        case Opcode::BEQ:
            c.branch = true;
            c.aluOp = ALUOp::SUB;
            break;
        default:
            break;
    }

    out.ctrl = c;
    out.valid = true;

    stall = false;
}
void EXStage::evaluate(PipelineRegisters& pipe, int& pc_next) {
    const ID_EX& in = pipe.id_ex;

    if (!in.valid) {
        pipe.ex_mem_next.valid = false;
        return;
    }

    // === Forwarding ===
    ForwardingDecision fwd =
        forwarding.resolve(in, pipe.ex_mem, pipe.mem_wb);

    int valA = in.val_rs;
    int valB = in.val_rt;

    if (fwd.A == ForwardSel::FROM_EX_MEM)
        valA = pipe.ex_mem.alu_result;
    else if (fwd.A == ForwardSel::FROM_MEM_WB)
        valA = pipe.mem_wb.ctrl.memToReg
                 ? pipe.mem_wb.mem_data
                 : pipe.mem_wb.alu_result;

    if (fwd.B == ForwardSel::FROM_EX_MEM)
        valB = pipe.ex_mem.alu_result;
    else if (fwd.B == ForwardSel::FROM_MEM_WB)
        valB = pipe.mem_wb.ctrl.memToReg
                 ? pipe.mem_wb.mem_data
                 : pipe.mem_wb.alu_result;

    EX_MEM& out = pipe.ex_mem_next;
    out.ctrl = in.ctrl;

    // === ALU ===
    int alu = 0;
    switch (in.ctrl.aluOp) {
        case ALUOp::ADD:
            alu = valA + (in.ctrl.aluSrcImm ? in.imm : valB);
            break;
        case ALUOp::SUB:
            alu = valA - valB;
            break;
        default:
            alu = 0;
    }

    out.alu_result = alu;

    // IMPORTANT: store forwarded value, not stale one
    out.val_rt = valB;

    out.zero = (alu == 0);
    out.branchTarget = in.pc + in.imm;
    out.valid = true;

    // Branch handling
    if (in.ctrl.branch && out.zero) {
        pc_next = out.branchTarget;
        pipe.if_id_next.valid = false;
        pipe.id_ex_next.valid = false;
    }
}


void MEMStage::evaluate(PipelineRegisters& pipe, Memory& mem) {
    const EX_MEM& in = pipe.ex_mem;

    if (!in.valid) {
        pipe.mem_wb_next.valid = false;
        return;
    }

    MEM_WB& out = pipe.mem_wb_next;
    out.ctrl = in.ctrl;
    out.alu_result = in.alu_result;

    if (in.ctrl.memRead) {
        out.mem_data = mem.read(in.alu_result);
    }
    if (in.ctrl.memWrite) {
        mem.writeNext(in.alu_result, in.val_rt);
    }

    out.valid = true;
}

void WBStage::evaluate(PipelineRegisters& pipe, RegisterFile& regs) {
    const MEM_WB& in = pipe.mem_wb;

    if (!in.valid) return;
    if (!in.ctrl.regWrite) return;

    int value = in.ctrl.memToReg ? in.mem_data : in.alu_result;
    int dest = in.ctrl.destReg;
    if (dest >= 0) regs.writeNext(dest, value);
}
