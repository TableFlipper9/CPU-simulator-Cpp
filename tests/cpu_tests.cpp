#include <iostream>
#include <vector>
#include <string>

#include "CPU.hpp"
#include "Instructions.hpp"

namespace {

int g_failures = 0;

#define EXPECT_EQ(actual, expected) do { \
    auto _a = (actual); \
    auto _e = (expected); \
    if (_a != _e) { \
        std::cerr << "  FAIL at " << __FILE__ << ":" << __LINE__ \
                  << " | got=" << _a << " expected=" << _e << "\n"; \
        ++g_failures; \
    } \
} while(0)

static Instruction I(Opcode op, int rs=0, int rt=0, int rd=0, int imm=0, int addr=0, const std::string& txt="") {
    Instruction ins;
    ins.op = op;
    ins.rs = rs;
    ins.rt = rt;
    ins.rd = rd;
    ins.imm = imm;
    ins.addr = addr;
    ins.raw_text = txt;
    return ins;
}

static void runCPU(CPU& cpu, int max_cycles) {
    for (int i = 0; i < max_cycles; ++i) {
        cpu.tick();
    }
}

static void runProgramAndDrain(CPU& cpu, const std::vector<Instruction>& prog) {
    cpu.loadProgram(prog);
    const int max_cycles = static_cast<int>(prog.size()) + 20;
    runCPU(cpu, max_cycles);
}

static void test_alu_forwarding() {
    std::cout << "[TEST] alu_forwarding\n";
    CPU cpu;
    std::vector<Instruction> p = {
        I(Opcode::ADDI, 0, 1, 0, 5, 0, "addi $1,$0,5"),
        I(Opcode::ADDI, 0, 2, 0, 7, 0, "addi $2,$0,7"),
        I(Opcode::ADD,  1, 2, 3, 0, 0, "add  $3,$1,$2"),
        I(Opcode::SUB,  3, 1, 4, 0, 0, "sub  $4,$3,$1"),
    };
    runProgramAndDrain(cpu, p);

    EXPECT_EQ(cpu.getReg(1), 5);
    EXPECT_EQ(cpu.getReg(2), 7);
    EXPECT_EQ(cpu.getReg(3), 12);
    EXPECT_EQ(cpu.getReg(4), 7);
}

static void test_load_use_stall_and_mem_to_ex_forward() {
    std::cout << "[TEST] load_use_stall_and_mem_to_ex_forward\n";
    CPU cpu;
    cpu.setMemWord(0, 42);

    std::vector<Instruction> p = {
        I(Opcode::LW,   0, 1, 0, 0, 0, "lw   $1,0($0)"),
        I(Opcode::ADD,  1, 1, 2, 0, 0, "add  $2,$1,$1"),
        I(Opcode::ADDI, 2, 3, 0, 1, 0, "addi $3,$2,1"),
    };
    runProgramAndDrain(cpu, p);

    EXPECT_EQ(cpu.getReg(1), 42);
    EXPECT_EQ(cpu.getReg(2), 84);
    EXPECT_EQ(cpu.getReg(3), 85);
}

static void test_store_data_forwarding() {
    std::cout << "[TEST] store_data_forwarding\n";
    CPU cpu;

    std::vector<Instruction> p = {
        I(Opcode::ADDI, 0, 1, 0, 99, 0, "addi $1,$0,99"),
        I(Opcode::SW,   0, 1, 0, 0,  0, "sw   $1,0($0)"),
        I(Opcode::LW,   0, 2, 0, 0,  0, "lw   $2,0($0)"),
    };
    runProgramAndDrain(cpu, p);

    EXPECT_EQ(cpu.getMemWord(0), 99);
    EXPECT_EQ(cpu.getReg(2), 99);
}

static void test_branch_taken_flush() {
    std::cout << "[TEST] branch_taken_flush\n";
    CPU cpu;

    // pc layout:
    // 0: r1=1
    // 1: r2=1
    // 2: beq r1,r2, +2  => target = 2+1+2 = 5
    // 3: (flushed)
    // 4: (flushed)
    // 5: r3=789
    std::vector<Instruction> p = {
        I(Opcode::ADDI, 0, 1, 0, 1,   0, "addi $1,$0,1"),
        I(Opcode::ADDI, 0, 2, 0, 1,   0, "addi $2,$0,1"),
        I(Opcode::BEQ,  1, 2, 0, 2,   0, "beq  $1,$2,2"),
        I(Opcode::ADDI, 0, 3, 0, 123, 0, "addi $3,$0,123"),
        I(Opcode::ADDI, 0, 3, 0, 456, 0, "addi $3,$0,456"),
        I(Opcode::ADDI, 0, 3, 0, 789, 0, "addi $3,$0,789"),
    };
    runProgramAndDrain(cpu, p);

    EXPECT_EQ(cpu.getReg(3), 789);
}

static void test_branch_not_taken_with_forward_to_branch() {
    std::cout << "[TEST] branch_not_taken_with_forward_to_branch\n";
    CPU cpu;

    std::vector<Instruction> p = {
        I(Opcode::ADDI, 0, 1, 0, 5, 0, "addi $1,$0,5"),
        I(Opcode::ADDI, 0, 2, 0, 6, 0, "addi $2,$0,6"),
        I(Opcode::SUB,  2, 1, 3, 0, 0, "sub  $3,$2,$1"), // r3=1
        I(Opcode::BEQ,  3, 0, 0, 1, 0, "beq  $3,$0,1"), // not taken
        I(Opcode::ADDI, 0, 4, 0, 11,0, "addi $4,$0,11"),
        I(Opcode::ADDI, 0, 4, 0, 22,0, "addi $4,$0,22"),
    };
    runProgramAndDrain(cpu, p);

    // Branch should not skip the first write to r4
    EXPECT_EQ(cpu.getReg(3), 1);
    EXPECT_EQ(cpu.getReg(4), 22);
}

static void test_jal_jr_control_hazards() {
    std::cout << "[TEST] jal_jr_control_hazards\n";
    CPU cpu;

    // main:
    // 0: jal 4      => $ra = 1, jump to 4
    // 1: addi r1,111 (runs after return)
    // 2: j 7        (skip over subroutine body so we don't call again)
    // 3: nop
    // subroutine:
    // 4: addi r2,222
    // 5: jr $ra
    // 6: nop
    // end:
    // 7: addi r3,333
    std::vector<Instruction> p = {
        I(Opcode::JAL, 0,0,0,0,4, "jal 4"),
        I(Opcode::ADDI,0,1,0,111,0, "addi $1,$0,111"),
        I(Opcode::J,   0,0,0,0,7, "j 7"),
        I(Opcode::NOP, 0,0,0,0,0, "nop"),
        I(Opcode::ADDI,0,2,0,222,0, "addi $2,$0,222"),
        I(Opcode::JR,  31,0,0,0,0, "jr $ra"),
        I(Opcode::NOP, 0,0,0,0,0, "nop"),
        I(Opcode::ADDI,0,3,0,333,0, "addi $3,$0,333"),
    };
    runProgramAndDrain(cpu, p);

    EXPECT_EQ(cpu.getReg(31), 1);   // link
    EXPECT_EQ(cpu.getReg(2), 222);  // ran in subroutine
    EXPECT_EQ(cpu.getReg(1), 111);  // ran after return
    EXPECT_EQ(cpu.getReg(3), 333);  // ran at end
}

static void test_branch_after_load_use_stall() {
    std::cout << "[TEST] branch_after_load_use_stall\n";
    CPU cpu;
    cpu.setMemWord(0, 0);

    // 0: lw   r1,0(r0)      => r1=0
    // 1: beq  r1,r0,+1      => taken, should skip pc=2
    // 2: addi r2,111        => flushed
    // 3: addi r2,222
    // This requires a load-use stall because the branch reads r1.
    std::vector<Instruction> p = {
        I(Opcode::LW,   0, 1, 0, 0, 0, "lw   $1,0($0)"),
        I(Opcode::BEQ,  1, 0, 0, 1, 0, "beq  $1,$0,1"),
        I(Opcode::ADDI, 0, 2, 0, 111, 0, "addi $2,$0,111"),
        I(Opcode::ADDI, 0, 2, 0, 222, 0, "addi $2,$0,222"),
    };
    runProgramAndDrain(cpu, p);

    EXPECT_EQ(cpu.getReg(1), 0);
    EXPECT_EQ(cpu.getReg(2), 222);
}

static void test_store_after_load_stall_and_forward() {
    std::cout << "[TEST] store_after_load_stall_and_forward\n";
    CPU cpu;
    cpu.setMemWord(0, 77);

    // 0: lw r1,0(r0)    => r1=77
    // 1: sw r1,4(r0)    => must store 77 (requires load-use stall)
    // 2: lw r2,4(r0)    => r2=77
    std::vector<Instruction> p = {
        I(Opcode::LW, 0, 1, 0, 0, 0, "lw   $1,0($0)"),
        I(Opcode::SW, 0, 1, 0, 4, 0, "sw   $1,4($0)"),
        I(Opcode::LW, 0, 2, 0, 4, 0, "lw   $2,4($0)"),
    };
    runProgramAndDrain(cpu, p);

    EXPECT_EQ(cpu.getReg(1), 77);
    EXPECT_EQ(cpu.getMemWord(4), 77);
    EXPECT_EQ(cpu.getReg(2), 77);
}

static void test_zero_register_immutable() {
    std::cout << "[TEST] zero_register_immutable\n";
    CPU cpu;

    std::vector<Instruction> p = {
        I(Opcode::ADDI, 0, 0, 0, 123, 0, "addi $0,$0,123"),
        I(Opcode::ADDI, 0, 1, 0, 5,   0, "addi $1,$0,5"),
        I(Opcode::ADD,  1, 0, 0, 0,   0, "add  $0,$1,$0"),
    };
    runProgramAndDrain(cpu, p);

    EXPECT_EQ(cpu.getReg(0), 0);
    EXPECT_EQ(cpu.getReg(1), 5);
}

} // namespace

int main() {
    test_alu_forwarding();
    test_load_use_stall_and_mem_to_ex_forward();
    test_store_data_forwarding();
    test_branch_taken_flush();
    test_branch_not_taken_with_forward_to_branch();
    test_jal_jr_control_hazards();
    test_branch_after_load_use_stall();
    test_store_after_load_stall_and_forward();
    test_zero_register_immutable();

    if (g_failures == 0) {
        std::cout << "\nALL TESTS PASSED\n";
        return 0;
    }

    std::cerr << "\nTESTS FAILED: " << g_failures << "\n";
    return 1;
}
