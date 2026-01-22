#include "Window.hpp"
#include <iostream>
#include <functional>
#include <unordered_map>

static sf::Texture gPipelineTexture;

static ImU32 ColorFromHash(uint32_t h, float alpha = 1.0f) {
    // More dispersed color generation than a small fixed palette.
    // We use HSV with a hashed hue so multiple instructions are very unlikely
    // to share the same color in a single iteration.
    //
    // Keep saturation/value in a readable range so text remains legible.
    const float hue = (float)h / 4294967296.0f; // [0, 1)
    const float sat = 0.60f + 0.35f * (((h >> 8) & 0xFF) / 255.0f);  // [0.60, 0.95]
    const float val = 0.70f + 0.25f * (((h >> 16) & 0xFF) / 255.0f); // [0.70, 0.95]

    ImVec4 rgb;
    ImGui::ColorConvertHSVtoRGB(hue, sat, val, rgb.x, rgb.y, rgb.z);
    rgb.w = alpha;
    return ImGui::ColorConvertFloat4ToU32(rgb);
}

static ImU32 InstrColor(const Instruction& instr, bool valid, float alpha = 1.0f) {
    if (!valid || instr.op == Opcode::NOP) {
        return ImGui::ColorConvertFloat4ToU32({0.65f, 0.65f, 0.65f, alpha});
    }
    // Use the raw text as the stable identity for coloring.
    // (If you later add a per-instruction unique ID/PC to *all* pipeline regs,
    //  switch the hash input to that for perfect uniqueness.)
    const uint32_t h = (uint32_t)std::hash<std::string>{}(instr.raw_text);
    return ColorFromHash(h, alpha);
}

static ImVec4 U32ToVec4(ImU32 c) {
    return ImGui::ColorConvertU32ToFloat4(c);
}

static std::string TruncateToWidth(const std::string& s, float maxW) {
    if (s.empty()) return s;
    if (ImGui::CalcTextSize(s.c_str()).x <= maxW) return s;

    // Reserve space for ellipsis.
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
                    // Single-step (user requested "tick on space")
                    if (!cpu.isHalted()) {
                        cpu.tick();
                        const auto& p = cpu.pipeline();
                        if (p.if_id.valid) executedHistory.push_back(p.if_id.rawInstr.raw_text);
                        else executedHistory.push_back("<empty>");
                    }
                } else if (key->scancode == sf::Keyboard::Scancode::Enter) {
                    // Toggle auto-run (nice to have)
                    running = !running;
                    runClock.restart();
                } else if (key->scancode == sf::Keyboard::Scancode::R) {
                    cpu.reset(true);
                    executedHistory.clear();
                }
            }
        }

        // Auto-run mode (optional)
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

        ImGui::SetNextWindowPos({workPos.x + cellW * 0, workPos.y + cellH * 0});
        ImGui::SetNextWindowSize({cellW * 7, cellH * 7});

        ImGui::Begin("Pipeline", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        // Header text FIRST so it can't end up clipped/hidden behind the title bar
        // or overlapped by the image.
        ImGui::Text("Clock: %d  PC: %d  State: %s", cpu.clock, cpu.pc,
            cpu.isHalted() ? "HALTED" : (running ? "RUN" : "PAUSE"));

        const auto& pipe = cpu.pipeline();

        // Colors for instructions that are *currently* in the pipeline.
        // We'll re-use these in the Executed list so only in-flight instructions
        // get highlighted; older ones remain plain white.
        std::unordered_map<std::string, ImU32> liveInstrColors;
        auto addLive = [&](const Instruction& instr, bool valid) {
            if (!valid || instr.op == Opcode::NOP) return;
            liveInstrColors[instr.raw_text] = InstrColor(instr, true, 1.0f);
        };
        addLive(pipe.if_id.rawInstr, pipe.if_id.valid);
        addLive(pipe.id_ex.rawInstr, pipe.id_ex.valid);
        addLive(pipe.ex_mem.rawInstr, pipe.ex_mem.valid);
        addLive(pipe.mem_wb.rawInstr, pipe.mem_wb.valid);

        ImGui::Separator();

        // Pipeline diagram fills the remaining space.
        // We then draw colored overlays on top of the *pipeline registers* inside the image.
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            sf::Vector2f size(avail.x, avail.y);
            ImGui::Image(gPipelineTexture, size);

            // Image rectangle in screen space.
            const ImVec2 imgMin = ImGui::GetItemRectMin();
            const ImVec2 imgMax = ImGui::GetItemRectMax();
            const ImVec2 imgSize(imgMax.x - imgMin.x, imgMax.y - imgMin.y);

            // These are normalized rectangles (0..1) over the diagram image that cover
            // the four pipeline registers: IF/ID, ID/EX, EX/MEM, MEM/WB.
            // The values were derived from the current diagram asset.
            struct Slot { const char* name; float x0, y0, x1, y1; const Instruction* instr; bool valid; };

            // NOTE: The raw image is 599x367. The register bars roughly occupy these x ranges:
            // IF/ID  : x ~ [118..138]
            // ID/EX  : x ~ [255..274]
            // EX/MEM : x ~ [377..396]
            // MEM/WB : x ~ [502..521]
            // We expand a bit and use a shared y range so the overlay feels consistent.
            // Make the overlays ~30% smaller vertically so they don't dominate the diagram.
            const float baseY0 = 70.0f / 367.0f;
            const float baseY1 = 305.0f / 367.0f;
            const float yCenter = (baseY0 + baseY1) * 0.5f;
            const float yHalf = (baseY1 - baseY0) * 0.5f * 0.70f; // 70% height
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

                const ImU32 border = InstrColor(*s.instr, s.valid, 1.0f);
                const ImU32 fill   = InstrColor(*s.instr, s.valid, 0.18f);

                dl->AddRectFilled(p0, p1, fill, rounding);
                dl->AddRect(p0, p1, border, rounding, 0, 2.0f);

                // Instruction label goes ABOVE the overlay.
                const std::string fullText = s.valid ? s.instr->raw_text : std::string("<empty>");

                // Allow the label to be wider than the register bar, but keep it inside the image.
                const float maxLabelW = imgSize.x - pad * 2.0f;
                const std::string clipped = TruncateToWidth(fullText, maxLabelW);

                // Slightly larger text so it remains readable.
                ImFont* font = ImGui::GetFont();
                const float fontSize = ImGui::GetFontSize() * 1.15f;
                const ImVec2 tsz = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, clipped.c_str());

                // Center the label above the bar and clamp to image bounds.
                float labelX = (p0.x + p1.x) * 0.5f - tsz.x * 0.5f;
                labelX = std::max(imgMin.x + pad, std::min(labelX, imgMax.x - pad - tsz.x));

                float labelY = p0.y - tsz.y - 3.0f;
                // Clamp so we never draw outside the image.
                if (labelY < imgMin.y + pad) labelY = imgMin.y + pad;

                // Slight background behind text for readability.
                const ImU32 bg = ImGui::ColorConvertFloat4ToU32({0.0f, 0.0f, 0.0f, 0.35f});
                dl->AddRectFilled({labelX - 2.0f, labelY - 1.0f}, {labelX + tsz.x + 2.0f, labelY + tsz.y + 1.0f}, bg, 3.0f);
                dl->AddText(font, fontSize, {labelX, labelY}, border, clipped.c_str());
            }
        }

        ImGui::End();

        // Swap the Instructions and Registers panes.
        // Instructions now lives in the right column (narrower), so we make it
        // more usable by adding scrollable regions + text wrapping.
        ImGui::SetNextWindowPos({workPos.x + cellW * 7, workPos.y + cellH * 0});
        ImGui::SetNextWindowSize({cellW * 3, cellH * 6});

        ImGui::Begin("Instructions", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Controls: SPACE=Step | ENTER=Run/Pause | R=Reset | ESC=Quit");
        ImGui::SliderFloat("Ticks/sec", &ticksPerSecond, 1.0f, 60.0f, "%.0f");

        ImGui::Separator();

        // Keep the lists readable in the narrower column.
        ImGui::PushTextWrapPos(0.0f);

        ImGui::Text("Program:");
        if (ImGui::BeginChild("##program", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 10.5f),
                              true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            const auto& prog = cpu.program();
            for (int i = 0; i < (int)prog.size(); ++i) {
                const bool isPC = (i == cpu.pc);
                if (isPC) {
                    ImGui::Text("-> %02d: %s", i, prog[i].raw_text.c_str());
                } else {
                    ImGui::Text("   %02d: %s", i, prog[i].raw_text.c_str());
                }
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

        ImGui::SetNextWindowPos({workPos.x + cellW * 0, workPos.y + cellH * 7});
        ImGui::SetNextWindowSize({cellW * 7, cellH * 3});

        ImGui::Begin("Registers", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        // Registers list gets scrollable space now that it is shorter vertically.
        if (ImGui::BeginChild("##regs", ImVec2(0, 0), true))
        {
            const auto& regs = cpu.regFile().getRegs();
            for (int i = 0; i < 32; ++i) {
                ImGui::Text("$%02d: %d", i, regs[i]);
            }
        }
        ImGui::EndChild();

        ImGui::End();

        ImGui::SetNextWindowPos({workPos.x + cellW * 7, workPos.y + cellH * 6});
        ImGui::SetNextWindowSize({cellW * 3, cellH * 4});

        ImGui::Begin("Memory", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        // Show a small window of memory (words)
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
