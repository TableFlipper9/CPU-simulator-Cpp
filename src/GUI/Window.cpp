#include "Window.hpp"
#include <iostream>
#include <functional>
#include <unordered_map>
#include <algorithm> // std::min/std::max

static sf::Texture gPipelineTexture;

static const ImU32 PIPELINE_COLORS[] = {
    IM_COL32(231,  76,  60, 255), // red
    IM_COL32( 46, 204, 113, 255), // green
    IM_COL32(241, 196,  15, 255), // yellow
    IM_COL32( 52, 152, 219, 255), // blue
    IM_COL32( 26, 188, 156, 255), // teal
    IM_COL32(155,  89, 182, 255), // purple
    IM_COL32(230, 126,  34, 255), // orange
    IM_COL32(255, 105, 180, 255), // pink
};
static const int PIPELINE_COLOR_COUNT = (int)(sizeof(PIPELINE_COLORS) / sizeof(PIPELINE_COLORS[0]));

static std::unordered_map<std::string, ImU32> gInstrColorCache;
static int gNextColorIndex = 0;

static ImU32 GetStableColorForKey(const std::string& key) {
    auto it = gInstrColorCache.find(key);
    if (it != gInstrColorCache.end()) return it->second;

    ImU32 c = PIPELINE_COLORS[gNextColorIndex % PIPELINE_COLOR_COUNT];
    gNextColorIndex++;
    gInstrColorCache[key] = c;
    return c;
}

static void ResetColorCache() {
    gInstrColorCache.clear();
    gNextColorIndex = 0;
}

static ImU32 WithAlpha(ImU32 c, float alpha01) {
    ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
    v.w = alpha01;
    return ImGui::ColorConvertFloat4ToU32(v);
}

static ImU32 EmptyColor(float alpha01 = 1.0f) {
    return ImGui::ColorConvertFloat4ToU32({0.65f, 0.65f, 0.65f, alpha01});
}

static ImVec4 U32ToVec4(ImU32 c) {
    return ImGui::ColorConvertU32ToFloat4(c);
}

static std::string TruncateToWidth(const std::string& s, float maxW) {
    if (s.empty()) return s;
    if (ImGui::CalcTextSize(s.c_str()).x <= maxW) return s;

    const char* ell = "...";
    const float ellW = ImGui::CalcTextSize(ell).x;
    if (ellW >= maxW) return std::string();

    std::string out = s;
    while (!out.empty()) {
        out.pop_back();
        std::string candidate = out + ell;
        if (ImGui::CalcTextSize(candidate.c_str()).x <= maxW) return candidate;
    }
    return std::string();
}

App::App(CPU& cpu)
    : window(sf::VideoMode({900u, 600u}),
             "MIPS Pipeline Simulator (ImGui + SFML 3)",
             sf::Style::Default)
    , cpu(cpu)
{
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML\n";
    }

    if (!gPipelineTexture.loadFromFile("resources/Mips_pipeLine.png")) {
        std::cerr << "ERROR: Could not load resources/Mips_pipeLine.png\n";
    }
    gPipelineTexture.setSmooth(true);

    ResetColorCache();
}

void App::run()
{
    sf::Clock deltaClock;
    sf::Clock runClock;
    std::vector<std::string> executedHistory;

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                float w = float(resized->size.x);
                float h = float(resized->size.y);
                window.setView(sf::View(sf::FloatRect({0, 0}, {w, h})));
            }

            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->scancode == sf::Keyboard::Scancode::Escape) {
                    window.close();
                } else if (key->scancode == sf::Keyboard::Scancode::Space) {
                    // Single-step
                    if (!cpu.isHalted()) {
                        cpu.tick();
                        const auto& p = cpu.pipeline();
                        if (p.if_id.valid) executedHistory.push_back(p.if_id.rawInstr.raw_text);
                        else executedHistory.push_back("<empty>");
                    }
                } else if (key->scancode == sf::Keyboard::Scancode::Enter) {
                    // Toggle auto-run
                    running = !running;
                    runClock.restart();
                } else if (key->scancode == sf::Keyboard::Scancode::R) {
                    cpu.reset(true);
                    executedHistory.clear();
                    ResetColorCache(); // reset palette assignments
                }
            }
        }

        // Auto-run mode
        if (running && !cpu.isHalted()) {
            const float dt = runClock.getElapsedTime().asSeconds();
            const float step = (ticksPerSecond <= 0.01f) ? 1.0f : (1.0f / ticksPerSecond);
            if (dt >= step) {
                cpu.tick();
                runClock.restart();
                const auto& p = cpu.pipeline();
                if (p.if_id.valid) executedHistory.push_back(p.if_id.rawInstr.raw_text);
                else executedHistory.push_back("<empty>");
            }
        } else if (cpu.isHalted()) {
            running = false;
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImVec2 workPos  = ImGui::GetMainViewport()->WorkPos;
        ImVec2 workSize = ImGui::GetMainViewport()->WorkSize;

        float cellW = workSize.x / 10.0f;
        float cellH = workSize.y / 10.0f;

        //PIPELINE
        ImGui::SetNextWindowPos({workPos.x + cellW * 0, workPos.y + cellH * 0});
        ImGui::SetNextWindowSize({cellW * 7, cellH * 7});

        ImGui::Begin("Pipeline", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Clock: %d  PC: %d  State: %s", cpu.clock, cpu.pc,
            cpu.isHalted() ? "HALTED" : (running ? "RUN" : "PAUSE"));

        const auto& pipe = cpu.pipeline();

        std::unordered_map<std::string, ImU32> liveInstrColors;
        auto addLive = [&](const Instruction& instr, bool valid) {
            if (!valid || instr.op == Opcode::NOP) return;
            liveInstrColors[instr.raw_text] = GetStableColorForKey(instr.raw_text);
        };
        addLive(pipe.if_id.rawInstr,  pipe.if_id.valid);
        addLive(pipe.id_ex.rawInstr,  pipe.id_ex.valid);
        addLive(pipe.ex_mem.rawInstr, pipe.ex_mem.valid);
        addLive(pipe.mem_wb.rawInstr, pipe.mem_wb.valid);

        ImGui::Separator();

        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            sf::Vector2f size(avail.x, avail.y);
            ImGui::Image(gPipelineTexture, size);

            const ImVec2 imgMin = ImGui::GetItemRectMin();
            const ImVec2 imgMax = ImGui::GetItemRectMax();
            const ImVec2 imgSize(imgMax.x - imgMin.x, imgMax.y - imgMin.y);

            struct Slot { const char* name; float x0, y0, x1, y1; const Instruction* instr; bool valid; };

            const float baseY0 = 70.0f / 367.0f;
            const float baseY1 = 305.0f / 367.0f;
            const float yCenter = (baseY0 + baseY1) * 0.5f;
            const float yHalf = (baseY1 - baseY0) * 0.5f * 0.70f;
            const float y0 = yCenter - yHalf;
            const float y1 = yCenter + yHalf;

            const Slot slots[] = {
                {"IF/ID",  (115.0f/599.0f), y0, (141.0f/599.0f), y1, &pipe.if_id.rawInstr,  pipe.if_id.valid},
                {"ID/EX",  (252.0f/599.0f), y0, (277.0f/599.0f), y1, &pipe.id_ex.rawInstr,  pipe.id_ex.valid},
                {"EX/MEM", (374.0f/599.0f), y0, (399.0f/599.0f), y1, &pipe.ex_mem.rawInstr, pipe.ex_mem.valid},
                {"MEM/WB", (499.0f/599.0f), y0, (524.0f/599.0f), y1, &pipe.mem_wb.rawInstr, pipe.mem_wb.valid},
            };

            ImDrawList* dl = ImGui::GetWindowDrawList();
            const float pad = 4.0f;
            const float rounding = 4.0f;

            for (const auto& s : slots) {
                ImVec2 p0(imgMin.x + s.x0 * imgSize.x, imgMin.y + s.y0 * imgSize.y);
                ImVec2 p1(imgMin.x + s.x1 * imgSize.x, imgMin.y + s.y1 * imgSize.y);

                const bool isLive = s.valid && (s.instr->op != Opcode::NOP);

                ImU32 base = EmptyColor(1.0f);
                if (isLive) {
                    auto it = liveInstrColors.find(s.instr->raw_text);
                    if (it != liveInstrColors.end()) base = it->second;
                }

                const ImU32 border = base;
                const ImU32 fill   = WithAlpha(base, 0.18f);

                dl->AddRectFilled(p0, p1, fill, rounding);
                dl->AddRect(p0, p1, border, rounding, 0, 2.0f);

                const std::string fullText = isLive ? s.instr->raw_text : std::string("<empty>");

                const float maxLabelW = imgSize.x - pad * 2.0f;
                const std::string clipped = TruncateToWidth(fullText, maxLabelW);

                ImFont* font = ImGui::GetFont();
                const float fontSize = ImGui::GetFontSize() * 1.15f;
                const ImVec2 tsz = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, clipped.c_str());

                float labelX = (p0.x + p1.x) * 0.5f - tsz.x * 0.5f;
                labelX = std::max(imgMin.x + pad, std::min(labelX, imgMax.x - pad - tsz.x));

                float labelY = p0.y - tsz.y - 3.0f;
                if (labelY < imgMin.y + pad) labelY = imgMin.y + pad;

                const ImU32 bg = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 0.35f});
                dl->AddRectFilled({labelX - 2.0f, labelY - 1.0f},
                                  {labelX + tsz.x + 2.0f, labelY + tsz.y + 1.0f},
                                  bg, 3.0f);

                dl->AddText(font, fontSize, {labelX, labelY}, border, clipped.c_str());
            }
        }

        ImGui::End();

        //INSTRUCTIONS
        ImGui::SetNextWindowPos({workPos.x + cellW * 7, workPos.y + cellH * 0});
        ImGui::SetNextWindowSize({cellW * 3, cellH * 6});

        ImGui::Begin("Instructions", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Controls: SPACE=Step | ENTER=Run/Pause | R=Reset | ESC=Quit");
        ImGui::SliderFloat("Ticks/sec", &ticksPerSecond, 1.0f, 60.0f, "%.0f");

        ImGui::Separator();

        ImGui::PushTextWrapPos(0.0f);

        ImGui::Text("Program:");
        if (ImGui::BeginChild("##program", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 10.5f),
                              true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            const auto& prog = cpu.program();
            for (int i = 0; i < (int)prog.size(); ++i) {
                const bool isPC = (i == cpu.pc);
                if (isPC) ImGui::Text("-> %02d: %s", i, prog[i].raw_text.c_str());
                else      ImGui::Text("   %02d: %s", i, prog[i].raw_text.c_str());
            }
        }
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::Text("Executed (most recent last):");
        if (ImGui::BeginChild("##history", ImVec2(0, 0),
                              true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            int n = (int)executedHistory.size();
            const int showLast = 40;
            int start = (n > showLast) ? (n - showLast) : 0;
            for (int i = start; i < n; ++i) {
                const auto it = liveInstrColors.find(executedHistory[i]);
                if (it != liveInstrColors.end()) {
                    ImGui::TextColored(U32ToVec4(it->second), "%s", executedHistory[i].c_str());
                } else {
                    ImGui::Text("%s", executedHistory[i].c_str());
                }
            }
        }
        ImGui::EndChild();

        ImGui::PopTextWrapPos();
        ImGui::End();

        // REGISTERS
        ImGui::SetNextWindowPos({workPos.x + cellW * 0, workPos.y + cellH * 7});
        ImGui::SetNextWindowSize({cellW * 7, cellH * 3});

        ImGui::Begin("Registers", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        if (ImGui::BeginChild("##regs", ImVec2(0, 0), true))
        {
            const auto& regs = cpu.regFile().getRegs();
            for (int i = 0; i < 32; ++i) {
                ImGui::Text("$%02d: %d", i, regs[i]);
            }
        }
        ImGui::EndChild();
        ImGui::End();

        //MEMORY 
        ImGui::SetNextWindowPos({workPos.x + cellW * 7, workPos.y + cellH * 6});
        ImGui::SetNextWindowSize({cellW * 3, cellH * 4});

        ImGui::Begin("Memory", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        int memWordsToShow = 64;
        ImGui::Text("Memory [0..%d] (word addressed)", memWordsToShow - 1);
        for (int i = 0; i < memWordsToShow && i < (int)cpu.memory().size(); ++i) {
            ImGui::Text("[%02d] = %d", i, cpu.getMemWord(i));
        }

        ImGui::End();

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}
