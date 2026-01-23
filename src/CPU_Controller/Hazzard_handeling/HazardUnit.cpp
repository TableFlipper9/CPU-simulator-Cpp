#include "HazardUnit.hpp"
#include "PipelineRegisters.hpp"

HazardResult HazardUnit::detect(const IF_ID& if_id, const ID_EX& id_ex) {
    HazardResult res;

    if (!if_id.valid || !id_ex.valid)
        return res;

    auto readsRt = [&](const Instruction& ins) -> bool {
        switch (ins.op) {
            // R-type ALU ops read rs and rt
            case Opcode::ADD:
            case Opcode::SUB:
            case Opcode::AND:
            case Opcode::OR:
            case Opcode::SLT:
                return true;

            // Branches read rs and rt
            case Opcode::BEQ:
            case Opcode::BNE:
                return true;

            // Store reads rt (store data) and rs (base)
            case Opcode::SW:
                return true;

            // JR reads only rs
            case Opcode::JR:
            default:
                return false;
        }
    };

    // Classic load-use hazard
    if (id_ex.ctrl.memRead) {
        const int loadReg = id_ex.ctrl.destReg;  // destination of LW
        const int rs = if_id.rawInstr.rs;
        const int rt = if_id.rawInstr.rt;

        const bool usesRs = (rs != 0); // rs==0 is still a read but never hazzards
        const bool usesRt = readsRt(if_id.rawInstr);

        if (loadReg != 0 && ((usesRs && loadReg == rs) || (usesRt && loadReg == rt))) {
            res.stall = true;
        }
    }

    return res;
}
