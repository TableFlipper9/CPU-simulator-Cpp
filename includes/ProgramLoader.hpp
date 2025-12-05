#pragma once
#include <string>
#include <vector>
#include "Instructions.hpp"

class ProgramLoader {
public:
    static std::vector<Instruction> loadFromFile(const std::string& path);
};
