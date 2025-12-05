#include "Memory.hpp"
#include <algorithm>

Memory::Memory(size_t words) {
    data.assign(words, 0);
}

int Memory::read(int addr) const {
    if (addr < 0 || (size_t)addr >= data.size()) return 0;
    return data[addr];
}

void Memory::writeNext(int addr, int value) {
    if (addr < 0 || (size_t)addr >= data.size()) return;
    pendingWrite = std::make_pair(addr, value);
}

void Memory::commit() {
    if (pendingWrite.has_value()) {
        auto [addr, val] = pendingWrite.value();
        if (addr >= 0 && (size_t)addr < data.size()) data[addr] = val;
    }
    pendingWrite.reset();
}
