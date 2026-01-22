#include "CPU.hpp"
#include "ProgramLoader.hpp"
#include "Window.hpp"

#include <iostream>
#include <filesystem>
#include <optional>

static std::vector<Instruction> defaultDemoProgram() {
    // Demo:
    // $2 = 20
    // $3 = 5
    // $1 = $2 + $3 = 25
    // $4 = $1 + 10 = 35
    // mem[0] = $4
    // $5 = mem[0] = 35
    // $6 = $5 - $4 = 0
    return {
        {Opcode::ADDI, 0, 2, 0, 20, 0, "addi $2, $0, 20"},
        {Opcode::ADDI, 0, 3, 0, 5,  0, "addi $3, $0, 5"},
        {Opcode::ADD,  2, 3, 1, 0,  0, "add $1, $2, $3"},
        {Opcode::ADDI, 1, 4, 0, 10, 0, "addi $4, $1, 10"},
        {Opcode::SW,   0, 4, 0, 0,  0, "sw $4, 0($0)"},
        {Opcode::LW,   0, 5, 0, 0,  0, "lw $5, 0($0)"},
        {Opcode::SUB,  5, 4, 6, 0,  0, "sub $6, $5, $4"},
    };
}

int main(int argc, char** argv) {
    // GUI placeholder (your current App::run returns immediately in this project);
    // we'll wire real stepping/running into the GUI later.
    App ui;
    ui.run();

    CPU cpu;

    std::vector<Instruction> program;

    // Resolve a program path robustly on multi-config builds (e.g. build/Debug/...) where
    // the current working directory may not be the project root.
    auto firstExisting = [](const std::vector<std::filesystem::path>& candidates)
            -> std::optional<std::filesystem::path> {
        for (const auto& p : candidates) {
            std::error_code ec;
            if (std::filesystem::exists(p, ec) && !ec) {
                return p;
            }
        }
        return std::nullopt;
    };

    std::optional<std::filesystem::path> resolved;
    if (argc >= 2) {
        resolved = std::filesystem::path(argv[1]);
    } else {
        const std::filesystem::path exePath = (argc >= 1) ? std::filesystem::path(argv[0]) : std::filesystem::path();
        const std::filesystem::path exeDir  = exePath.has_parent_path() ? exePath.parent_path() : std::filesystem::current_path();

        // Try: CWD, alongside the exe, and typical build folder parents.
        resolved = firstExisting({
            std::filesystem::path("program.txt"),
            exeDir / "program.txt",
            exeDir / ".." / "program.txt",
            exeDir / ".." / ".." / "program.txt",
            exeDir / ".." / ".." / ".." / "program.txt",
        });
    }

    if (resolved && std::filesystem::exists(*resolved)) {
        try {
            program = ProgramLoader::loadFromFile(resolved->string());
            std::cout << "Loaded program from: " << resolved->string() << " (" << program.size() << " instructions)\n";
        } catch (const std::exception& e) {
            std::cerr << "Failed to load '" << resolved->string() << "': " << e.what() << "\n";
            std::cerr << "Falling back to built-in demo program.\n";
            program = defaultDemoProgram();
        }
    } else {
        std::cout << "Program file not found.\n";
        std::cout << "Tip: pass a path as argv[1], e.g. SCS_CPU_Simulator.exe myprog.txt\n";
        std::cout << "Falling back to built-in demo program.\n";
        program = defaultDemoProgram();
    }

    cpu.loadProgram(program);

    // Run long enough to retire everything through the pipeline.
    int steps = static_cast<int>(program.size()) + 8;
    for (int i = 0; i < steps; i++) {
        std::cout << "=== Tick " << i << " ===\n";
        cpu.tick();
        cpu.dumpPipeline();
        cpu.dumpRegisters();
        std::cout << "-------------------------\n";
    }

    return 0;
}
