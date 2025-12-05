#include "Window.hpp"
#include <iostream>
#include <imgui.h>
#include <imgui-SFML.h>

sf::Texture pipelineTexture;

App::App()
    : window(sf::VideoMode({900u, 600u}),
             "MIPS Pipeline Simulator (ImGui + SFML 3)",
             sf::Style::Default)
{
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML\n";
    }

    if (!pipelineTexture.loadFromFile("resources/Mips_pipeLine.png")) {
        std::cerr << "ERROR: Could not load resources/Mips_pipeLine.png\n";
    }
    pipelineTexture.setSmooth(true);
}

void App::run()
{
    sf::Clock deltaClock;
    std::vector<std::string> instructionSet = {
        "Load instruction",
        "Add R1, R2, R3",
        "Store instruction",
        "Branch taken",
        "NOP",
        "JUMP",
        "Addi R1, R2"
    };
    int x = 0;

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

            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->scancode == sf::Keyboard::Scancode::Escape)
                    window.close();
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->scancode == sf::Keyboard::Scancode::Escape) {
                    window.close();
                }
                else if (key->scancode == sf::Keyboard::Scancode::Space) {
                    // instructions.insert(instructions.begin(),instructionSet[x]);
                    instructions.push_back(instructionSet[x]);
                    x = (x + 1) % instructionSet.size();
                }
            }
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
            ImGui::Image(pipelineTexture, size);
        }

        ImGui::End();

        ImGui::SetNextWindowPos({workPos.x + cellW * 0, workPos.y + cellH * 7});
        ImGui::SetNextWindowSize({cellW * 7, cellH * 3});

        ImGui::Begin("Instructions", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        int n = instructions.size();
        for (int i = 0; i < n; ++i)
        {
            int age = n - 1 - i; 

            sf::Color color;
            if (age < recencyColors.size())
                color = recencyColors[age];
            else
                color = sf::Color::White;

            ImVec4 col = ImVec4(color.r / 255.f, color.g / 255.f, color.b / 255.f, 1.0f);
            ImGui::TextColored(col, "%s", instructions[i].c_str());
        }

        ImGui::End();

        ImGui::SetNextWindowPos({workPos.x + cellW * 7, workPos.y + cellH * 0});
        ImGui::SetNextWindowSize({cellW * 3, cellH * 6});

        ImGui::Begin("Registers", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        for (int i = 0; i < 32; ++i)
            ImGui::Text("R%d: %d", i, 0);

        ImGui::End();

        ImGui::SetNextWindowPos({workPos.x + cellW * 7, workPos.y + cellH * 6});
        ImGui::SetNextWindowSize({cellW * 3, cellH * 4});

        ImGui::Begin("Memory", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Memory dump here...");

        ImGui::End();

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}
