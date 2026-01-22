#include "HazardUnit.hpp"
#include "PipelineRegisters.hpp"

HazardResult HazardUnit::detect(const IF_ID& if_id, const ID_EX& id_ex) {
    HazardResult res;

    if (!if_id.valid || !id_ex.valid)
        return res;

    if (id_ex.ctrl.memRead) {
        int loadReg = id_ex.rt;

        if (loadReg != 0 &&
            (loadReg == if_id.rs || loadReg == if_id.rt)) {
            res.stall = true;
        }
    }

    return res;
}
