#include "ProgramLoader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <algorithm>

static inline std::string trim(std::string s) {
    auto notSpace = [](unsigned char c){ return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

static inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); });
    return s;
}

static inline std::string stripPunct(std::string s) {
    // remove trailing commas etc.
    while (!s.empty() && (s.back() == ',')) s.pop_back();
    return s;
}

static int parseReg(const std::string& tok, int lineNo) {
    std::string s = stripPunct(tok);
    if (!s.empty() && s[0] == '$') s.erase(0,1);
    if (s.empty()) throw std::runtime_error("Line " + std::to_string(lineNo) + ": missing register");
    // Allow names like 
    for(char c: s) {
        if (!std::isdigit((unsigned char)c) && c!='-' ) {
            throw std::runtime_error("Line " + std::to_string(lineNo) + ": invalid register token: " + tok);
        }
    }
    int r = std::stoi(s);
    if (r < 0 || r > 31) throw std::runtime_error("Line " + std::to_string(lineNo) + ": register out of range: " + tok);
    return r;
}

static int parseImm(const std::string& tok, int lineNo) {
    std::string s = stripPunct(tok);
    if (s.empty()) throw std::runtime_error("Line " + std::to_string(lineNo) + ": missing immediate");
    try { return std::stoi(s); }
    catch(...) { throw std::runtime_error("Line " + std::to_string(lineNo) + ": invalid immediate: " + tok); }
}

static void parseMemOperand(const std::string& tok, int& imm, int& base, int lineNo) {
    // format: imm(base) e.g. 8($2)
    std::string s = stripPunct(tok);
    auto lp = s.find('(');
    auto rp = s.find(')');
    if (lp == std::string::npos || rp == std::string::npos || rp < lp) {
        throw std::runtime_error("Line " + std::to_string(lineNo) + ": invalid memory operand (expected imm(base)): " + tok);
    }
    std::string immStr = s.substr(0, lp);
    std::string baseStr = s.substr(lp+1, rp-lp-1);
    imm = immStr.empty() ? 0 : parseImm(immStr, lineNo);
    base = parseReg(baseStr, lineNo);
}

std::vector<Instruction> ProgramLoader::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open program file: " + path);
    }

    std::vector<Instruction> program;
    std::string line;
    int lineNo = 0;

    while (std::getline(in, line)) {
        lineNo++;
        // strip comments (# or //)
        auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        auto sl = line.find("//");
        if (sl != std::string::npos) line = line.substr(0, sl);

        line = trim(line);
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string mnem;
        iss >> mnem;
        mnem = toLower(mnem);

        Instruction ins;
        ins.raw_text = line;

        if (mnem == "nop") {
            ins.op = Opcode::NOP;
        } else if (mnem == "add" || mnem == "sub" || mnem == "and" || mnem == "or" || mnem == "xor" || mnem == "slt") {
            std::string rd, rs, rt;
            if (!(iss >> rd >> rs >> rt)) {
                throw std::runtime_error("Line " + std::to_string(lineNo) + ": expected rd rs rt");
            }
            ins.rd = parseReg(rd, lineNo);
            ins.rs = parseReg(rs, lineNo);
            ins.rt = parseReg(rt, lineNo);
            if (mnem == "add") ins.op = Opcode::ADD;
            else if (mnem == "sub") ins.op = Opcode::SUB;
            else if (mnem == "and") ins.op = Opcode::AND;
            else if (mnem == "or") ins.op = Opcode::OR;
            else if (mnem == "xor") ins.op = Opcode::XOR;
            else ins.op = Opcode::SLT;
        } else if (mnem == "jr") {
            std::string rs;
            if (!(iss >> rs)) throw std::runtime_error("Line " + std::to_string(lineNo) + ": expected rs");
            ins.op = Opcode::JR;
            ins.rs = parseReg(rs, lineNo);
        } else if (mnem == "addi" || mnem == "andi" || mnem == "ori") {
            std::string rt, rs, imm;
            if (!(iss >> rt >> rs >> imm)) throw std::runtime_error("Line " + std::to_string(lineNo) + ": expected rt rs imm");
            ins.rt = parseReg(rt, lineNo);
            ins.rs = parseReg(rs, lineNo);
            ins.imm = parseImm(imm, lineNo);
            if (mnem == "addi") ins.op = Opcode::ADDI;
            else if (mnem == "andi") ins.op = Opcode::ANDI;
            else ins.op = Opcode::ORI;
        } else if (mnem == "lw" || mnem == "sw") {
            std::string rt, mem;
            if (!(iss >> rt >> mem)) throw std::runtime_error("Line " + std::to_string(lineNo) + ": expected rt imm(base)");
            int imm=0, base=0;
            parseMemOperand(mem, imm, base, lineNo);
            ins.rt = parseReg(rt, lineNo);
            ins.rs = base;
            ins.imm = imm;
            ins.op = (mnem == "lw") ? Opcode::LW : Opcode::SW;
        } else if (mnem == "beq" || mnem == "bne") {
            std::string rs, rt, target;
            if (!(iss >> rs >> rt >> target)) throw std::runtime_error("Line " + std::to_string(lineNo) + ": expected rs rt targetIndex");
            ins.rs = parseReg(rs, lineNo);
            ins.rt = parseReg(rt, lineNo);
            ins.imm = parseImm(target, lineNo);
            int pc = (int)program.size();
            int targetPc = ins.imm;
            ins.imm = targetPc - (pc + 1);
            ins.op = (mnem == "beq") ? Opcode::BEQ : Opcode::BNE;
        } else if (mnem == "j" || mnem == "jal") {
            std::string target;
            if (!(iss >> target)) throw std::runtime_error("Line " + std::to_string(lineNo) + ": expected targetIndex");
            ins.addr = parseImm(target, lineNo);
            ins.op = (mnem == "j") ? Opcode::J : Opcode::JAL;
        } else {
            throw std::runtime_error("Line " + std::to_string(lineNo) + ": unknown mnemonic: " + mnem);
        }

        program.push_back(ins);
    }

    return program;
}
