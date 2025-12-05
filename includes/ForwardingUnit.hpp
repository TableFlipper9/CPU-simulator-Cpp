#pragma once
#include "PipelineRegisters.hpp"

enum class ForwardSel {
    NONE,
    FROM_EX_MEM,
    FROM_MEM_WB
};

struct ForwardingDecision {
    ForwardSel A = ForwardSel::NONE;
    ForwardSel B = ForwardSel::NONE;
};

class ForwardingUnit {
public:
    ForwardingDecision resolve(
        const ID_EX& id_ex,
        const EX_MEM& ex_mem,
        const MEM_WB& mem_wb
    );
};
