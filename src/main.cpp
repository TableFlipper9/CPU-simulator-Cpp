#include "CPU.hpp"
#include <iostream>
#include "Window.hpp"


int main() {
    App ui;
    ui.run();
    
    CPU cpu;

    // Example program ( should be assembly ?)
    // r1 = r2 + r3
    // r4 = r1 + 10 (ADDI)
    // store r4 -> mem[0]
    // load mem[0] -> r5
    // r6 = r5 - r4
    std::vector<Instruction> program = {
        {Opcode::ADD, 2, 3, 1, 0, 0, "add $1, $2, $3"},
        {Opcode::ADDI, 1, 4, 0, 10, 0, "addi $4, $1, 10"},
        {Opcode::SW, 4, 0, 0, 0, 0, "sw $4, 0($0)"},
        {Opcode::LW, 0, 5, 0, 0, 0, "lw $5, 0($0)"},
        {Opcode::SUB, 5, 4, 6, 0, 0, "sub $6, $5, $4"},
    };

    cpu.loadProgram(program);

    std::vector<Instruction> init = {
        {Opcode::ADDI, 0, 2, 0, 20, 0, "addi $2, $0, 20"},
        {Opcode::ADDI, 0, 3, 0, 5,  0, "addi $3, $0, 5"}
    };

    init.insert(init.end(), program.begin(), program.end());
    cpu.loadProgram(init);

    int steps = 12;
    for (int i=0;i<steps;i++) {
        std::cout << "=== Tick " << i << " ===\n";
        cpu.tick();
        cpu.dumpPipeline();
        cpu.dumpRegisters();
        std::cout << "-------------------------\n";
    }

    return 0;
}

