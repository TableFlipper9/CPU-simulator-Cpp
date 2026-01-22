#include "RegisterFile.hpp"

RegisterFile::RegisterFile() {
    regs.fill(0);
    regs[0] = 0;
}

void RegisterFile::reset() {
    regs.fill(0);
    pendingWrite.reset();
    regs[0] = 0;
}

int RegisterFile::read(int idx) const {
    if (idx < 0 || idx >= 32) return 0;
    return regs[idx];
}

void RegisterFile::writeNext(int idx, int value) {
    if (idx <= 0 || idx >= 32) return; 
    pendingWrite = std::make_pair(idx, value);
}

void RegisterFile::commit() {
    if (pendingWrite.has_value()) {
        auto [idx, val] = pendingWrite.value();
        if (idx > 0 && idx < 32) regs[idx] = val;
    }
    pendingWrite.reset();
    regs[0] = 0;
}
