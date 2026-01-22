#pragma once
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "CPU.hpp"

class App {
public:
    explicit App(CPU& cpu);
    void run();

private:
    sf::RenderWindow window;
    sf::Texture pipelineTexture;
    CPU& cpu;

    // UI run control
    bool running = false;
    float ticksPerSecond = 4.0f;

    std::vector<sf::Color> recencyColors = {
        sf::Color::Red,   
        sf::Color::Blue, 
        sf::Color::Green, 
        sf::Color::Yellow,
        sf::Color(128, 0, 128) 
    };

};
