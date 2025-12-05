#pragma once
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

class App {
public:
    App();
    void run();

private:
    sf::RenderWindow window;
    sf::Texture pipelineTexture;
    std::vector<std::string> instructions;
    std::vector<sf::Color> recencyColors = {
        sf::Color::Red,   
        sf::Color::Blue, 
        sf::Color::Green, 
        sf::Color::Yellow,
        sf::Color(128, 0, 128) 
    };

};
