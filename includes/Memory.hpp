 #pragma once
 #include <vector>
// #include <cstdint>
// #include <string>
// #include <iostream>
#include <optional>

// class Memory {
// private:
//     std::vector<uint8_t> mem; 
//     uint32_t baseAddress;             

// public:
//     explicit Memory(uint32_t size = 4096, uint32_t base = 0x00000000);

//     uint8_t readByte(uint32_t address) const;
//     void writeByte(uint32_t address, uint8_t value);

//     // void loadP(const std::vector<uint32_t>& instructions, uint32_t startAddr);
//     // uint32_t fetchInstruction(uint32_t pc) const;

//     void reset();

//     inline uint32_t getBaseAddress() const { return baseAddress; }
// };

class Memory {
public:
    Memory(size_t words = 1024);

    // Clear memory to 0 and discard any pending write
    void reset();

    int read(int addr) const;
    void writeNext(int addr, int value);
    void commit(); 

    size_t size() const { return data.size(); }

private:
    std::vector<int> data;
    std::optional<std::pair<int,int>> pendingWrite;
};

