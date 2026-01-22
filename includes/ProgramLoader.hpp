#pragma once
#include <string>
#include <vector>
#include "Instructions.hpp"

// Simple program loader for development.
// Supports a small assembly-like syntax without labels (numeric targets only).
// Examples:
//   addi $2, $0, 20
//   add  $1, $2, $3
//   sw   $4, 0($0)
//   lw   $5, 0($0)
//   beq  $1, $2, 10
//   j    6
class ProgramLoader {
public:
    static std::vector<Instruction> loadFromFile(const std::string& path);
};
