#include "ForwardingUnit.hpp"

ForwardingDecision ForwardingUnit::resolve(
    const ID_EX& id_ex,
    const EX_MEM& ex_mem,
    const MEM_WB& mem_wb
) {
    ForwardingDecision fwd;

    if (ex_mem.valid && ex_mem.ctrl.regWrite && ex_mem.ctrl.destReg != 0) {
        if (ex_mem.ctrl.destReg == id_ex.rs)
            fwd.A = ForwardSel::FROM_EX_MEM;
        if (ex_mem.ctrl.destReg == id_ex.rt)
            fwd.B = ForwardSel::FROM_EX_MEM;
    }

    if (mem_wb.valid && mem_wb.ctrl.regWrite && mem_wb.ctrl.destReg != 0) {
        if (fwd.A == ForwardSel::NONE &&
            mem_wb.ctrl.destReg == id_ex.rs)
            fwd.A = ForwardSel::FROM_MEM_WB;

        if (fwd.B == ForwardSel::NONE &&
            mem_wb.ctrl.destReg == id_ex.rt)
            fwd.B = ForwardSel::FROM_MEM_WB;
    }

    return fwd;
}
