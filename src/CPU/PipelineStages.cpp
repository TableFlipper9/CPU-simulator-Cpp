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


void IDStage::evaluate(PipelineRegisters& pipe, const RegisterFile& regs, bool stall) {
    if (stall) {
        // Insert bubble (NOP) into ID/EX; IF/ID is held by IF stage.
        pipe.id_ex_next = ID_EX{};
        pipe.id_ex_next.valid = false;
        return;
    }
    const IF_ID& in = pipe.if_id;

    if (!in.valid) {
        pipe.id_ex_next.valid = false;
        return;
    }

    const Instruction& di = in.rawInstr;
    ID_EX& out = pipe.id_ex_next;

    // Keep the decoded instruction for UI/debugging.
    out.rawInstr = di;

    out.pc = in.pc;
    out.rs = di.rs;
    out.rt = di.rt;
    out.imm = di.imm;
    out.addr = di.addr;

	// Register file timing model note:
	// In real MIPS, WB writes can be visible to ID reads in the same cycle
	// (write-first). Our simulator commits at end-of-tick, so we model that
	// behavior explicitly by allowing MEM/WB to bypass into ID.
	auto readWithWbBypass = [&](int idx) -> int {
	    int v = regs.read(idx);
	    if (pipe.mem_wb.valid && pipe.mem_wb.ctrl.regWrite && pipe.mem_wb.ctrl.destReg == idx && idx != 0) {
	        v = pipe.mem_wb.ctrl.memToReg ? pipe.mem_wb.mem_data : pipe.mem_wb.alu_result;
	    }
	    return v;
	};
	out.val_rs = readWithWbBypass(di.rs);
	out.val_rt = readWithWbBypass(di.rt);

    ControlSignals c;
    c.aluOp = ALUOp::NONE;
    c.destReg = -1;

    switch (di.op) {
        // === R-type ===
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
        case Opcode::AND:
            c.regWrite = true;
            c.aluOp = ALUOp::AND;
            c.destReg = di.rd;
            break;
        case Opcode::OR:
            c.regWrite = true;
            c.aluOp = ALUOp::OR;
            c.destReg = di.rd;
            break;
        case Opcode::SLT:
            c.regWrite = true;
            c.aluOp = ALUOp::SLT;
            c.destReg = di.rd;
            break;
        case Opcode::JR:
            c.jump = JumpType::JR;
            break;

        // === I-type ===
        case Opcode::ADDI:
            c.regWrite = true;
            c.aluOp = ALUOp::ADD;
            c.aluSrcImm = true;
            c.destReg = di.rt;
            break;
        case Opcode::ANDI:
            c.regWrite = true;
            c.aluOp = ALUOp::AND;
            c.aluSrcImm = true;
            c.destReg = di.rt;
            break;
        case Opcode::ORI:
            c.regWrite = true;
            c.aluOp = ALUOp::OR;
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
            c.branch = BranchType::BEQ;
            c.aluOp = ALUOp::SUB; // compare rs - rt
            break;
        case Opcode::BNE:
            c.branch = BranchType::BNE;
            c.aluOp = ALUOp::SUB; // compare rs - rt
            break;

        // === J-type ===
        case Opcode::J:
            c.jump = JumpType::J;
            break;
        case Opcode::JAL:
            c.jump = JumpType::JAL;
            c.regWrite = true;
            c.destReg = 31; // $ra
            break;

        default:
            break;
    }

    out.ctrl = c;
    out.valid = true;

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

    // MEM->EX forwarding for loads:
    // The instruction currently in EX/MEM may be a load. Its data becomes available
    // in MEM this cycle, which we model by running MEMStage before EXStage and
    // reading pipe.mem_wb_next.mem_data here.
    const bool memStageLoadAvail =
        pipe.ex_mem.valid && pipe.ex_mem.ctrl.memRead &&
        pipe.ex_mem.ctrl.regWrite &&
        pipe.ex_mem.ctrl.destReg != 0 &&
        pipe.mem_wb_next.valid && pipe.mem_wb_next.ctrl.memToReg;
    const int memStageLoadVal = pipe.mem_wb_next.mem_data;

    if (fwd.A == ForwardSel::FROM_EX_MEM)
        valA = pipe.ex_mem.alu_result;
    else if (fwd.A == ForwardSel::FROM_MEM_WB)
        valA = pipe.mem_wb.ctrl.memToReg
                 ? pipe.mem_wb.mem_data
                 : pipe.mem_wb.alu_result;

    // Override with MEM-stage load forwarding if applicable.
    if (memStageLoadAvail && pipe.ex_mem.ctrl.destReg == in.rs) {
        valA = memStageLoadVal;
    }

    if (fwd.B == ForwardSel::FROM_EX_MEM)
        valB = pipe.ex_mem.alu_result;
    else if (fwd.B == ForwardSel::FROM_MEM_WB)
        valB = pipe.mem_wb.ctrl.memToReg
                 ? pipe.mem_wb.mem_data
                 : pipe.mem_wb.alu_result;

    // Override with MEM-stage load forwarding if applicable.
    if (memStageLoadAvail && pipe.ex_mem.ctrl.destReg == in.rt) {
        valB = memStageLoadVal;
    }

    EX_MEM& out = pipe.ex_mem_next;
    // Keep instruction for UI/debugging.
    out.rawInstr = in.rawInstr;
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
        case ALUOp::AND:
            alu = valA & (in.ctrl.aluSrcImm ? in.imm : valB);
            break;
        case ALUOp::OR:
            alu = valA | (in.ctrl.aluSrcImm ? in.imm : valB);
            break;
        case ALUOp::SLT:
            alu = (valA < valB) ? 1 : 0;
            break;
        default:
            alu = 0;
    }

    out.alu_result = alu;

    // IMPORTANT: store forwarded value, not stale one
    out.val_rt = valB;

    out.zero = (alu == 0);
    out.branchTarget = in.pc + 1 + in.imm;
    out.valid = true;

    // === Control flow handling (basic) ===
// We resolve branches/jumps in EX. This means IF and ID have already speculatively
// fetched/decoded the next sequential instruction, so we flush them on redirect.
bool takeBranch = false;
if (in.ctrl.branch == BranchType::BEQ) {
    takeBranch = out.zero;
} else if (in.ctrl.branch == BranchType::BNE) {
    takeBranch = !out.zero;
}

if (takeBranch) {
    pc_next = out.branchTarget;
    pipe.if_id_next.valid = false;
    pipe.id_ex_next.valid = false;
}

// J / JAL use absolute target (instruction index in this simulator)
if (in.ctrl.jump == JumpType::J || in.ctrl.jump == JumpType::JAL) {
    pc_next = in.addr;
    pipe.if_id_next.valid = false;
    pipe.id_ex_next.valid = false;

    // For JAL, write return address (pc+1) into $ra via the normal WB path.
    // We carry it in alu_result.
    if (in.ctrl.jump == JumpType::JAL) {
        out.alu_result = in.pc + 1;
    }
}

// JR: jump to address stored in rs (valA)
if (in.ctrl.jump == JumpType::JR) {
    pc_next = valA;
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
    // Keep instruction for UI/debugging.
    out.rawInstr = in.rawInstr;
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
