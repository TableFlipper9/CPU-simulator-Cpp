#include "Window.hpp"
#include <iostream>

static sf::Texture gPipelineTexture;

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

        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            sf::Vector2f size(avail.x, avail.y);
            ImGui::Image(gPipelineTexture, size);
        }

        // A compact live state overlay
        ImGui::SetCursorPos({10, 10});
        ImGui::Text("Clock: %d  PC: %d  State: %s", cpu.clock, cpu.pc,
            cpu.isHalted() ? "HALTED" : (running ? "RUN" : "PAUSE"));

        const auto& pipe = cpu.pipeline();
        ImGui::Text("IF/ID:  %s", pipe.if_id.valid ? pipe.if_id.rawInstr.raw_text.c_str() : "<empty>");
        ImGui::Text("ID/EX:  %s", pipe.id_ex.valid ? pipe.id_ex.rawInstr.raw_text.c_str() : "<empty>");
        ImGui::Text("EX/MEM: %s", pipe.ex_mem.valid ? pipe.ex_mem.rawInstr.raw_text.c_str() : "<empty>");
        ImGui::Text("MEM/WB: %s", pipe.mem_wb.valid ? pipe.mem_wb.rawInstr.raw_text.c_str() : "<empty>");

        ImGui::End();

        ImGui::SetNextWindowPos({workPos.x + cellW * 0, workPos.y + cellH * 7});
        ImGui::SetNextWindowSize({cellW * 7, cellH * 3});

        ImGui::Begin("Instructions", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Controls: SPACE=Step | ENTER=Run/Pause | R=Reset | ESC=Quit");
        ImGui::SliderFloat("Ticks/sec", &ticksPerSecond, 1.0f, 60.0f, "%.0f");

        ImGui::Separator();
        ImGui::Text("Program:");
        const auto& prog = cpu.program();
        for (int i = 0; i < (int)prog.size(); ++i) {
            const bool isPC = (i == cpu.pc);
            if (isPC) {
                ImGui::Text("-> %02d: %s", i, prog[i].raw_text.c_str());
            } else {
                ImGui::Text("   %02d: %s", i, prog[i].raw_text.c_str());
            }
        }

        ImGui::Separator();
        ImGui::Text("Executed (most recent last):");
        int n = (int)executedHistory.size();
        const int showLast = 12;
        int start = (n > showLast) ? (n - showLast) : 0;
        for (int i = start; i < n; ++i) {
            ImGui::Text("%s", executedHistory[i].c_str());
        }

        ImGui::End();

        ImGui::SetNextWindowPos({workPos.x + cellW * 7, workPos.y + cellH * 0});
        ImGui::SetNextWindowSize({cellW * 3, cellH * 6});

        ImGui::Begin("Registers", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        const auto& regs = cpu.regFile().getRegs();
        for (int i = 0; i < 32; ++i) {
            ImGui::Text("$%02d: %d", i, regs[i]);
        }

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
