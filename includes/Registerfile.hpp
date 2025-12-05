#pragma once
#include <array>
#include <optional>

class RegisterFile {
public:
    RegisterFile();

    int read(int idx) const; 
    void writeNext(int idx, int value); 
    void commit(); 

    const std::array<int,32>& getRegs() const { return regs; }

private:
    std::array<int,32> regs;
    std::optional<std::pair<int,int>> pendingWrite;
};
