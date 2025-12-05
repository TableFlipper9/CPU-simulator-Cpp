#include "PipelineRegisters.hpp"

void PipelineRegisters::clearNext() {
    if_id_next = IF_ID{};
    id_ex_next = ID_EX{};
    ex_mem_next = EX_MEM{};
    mem_wb_next = MEM_WB{};
}
