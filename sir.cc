/*sir.cc -- SIR simulation written in C++20 with SFMl.
 *by Jacob Lagares Pozo (@jlagarespo on github)
 *
 *This program was written in April 3rd 2020, during the coronavirus (SARS-CoV-2 causant of the COVID-19 disease)
 *outbreak.
 *
 *SIR stands for Susceptible, Infected and Removed. It is used in epidemiology to model the spread of an
 *infection. More information on https://en.wikipedia.org/wiki/Compartmental_models_in_epidemiology#The_SIR_model.
 */

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <random>
#include <memory>
#include <map>



std::default_random_engine gen;

float speed = 3.0f;
float infection_radius = 100.0f;
float infection_chance = 0.0125f;
float infection_duration = 10.0f;



/*========================================================
 *SIMULATION CLASSES
 */

class sir;

class person
{
public:
    enum class state {
        susceptible,
        infected,
        removed
    };
    
    person(sf::Vector2f position, float speed, std::map<person::state, int>& stats): m_position(position), random(-0.2, 0.2), m_circle(20.0f), m_speed(speed), m_stats(stats) {
        this->m_direction = std::uniform_real_distribution<float>(-1000, 1000)(gen);
        phase(state::susceptible);
    }
    
    void tick(std::vector<std::unique_ptr<person>>& others, float size) {
        // Just roam around
        this->m_direction += random(gen);
        this->move({std::sin(this->m_direction) * this->m_speed, std::cos(this->m_direction) * this->m_speed}, size);

        // ...and infect others ;)
        if (this->m_state == state::infected) {
            for (auto& victim: others)
                if (victim->get_state() == state::susceptible && infect_random(gen) < infection_chance)
                    if (this->is_in_radius(victim))
                        victim->infect();
        }
        
        // Die or recover
        if (this->m_clock.getElapsedTime().asSeconds() > infection_duration && this->m_state == state::infected) {
            phase(state::removed);
        }
    }
    
    void draw(sf::RenderWindow& window) {
        if (this->m_state == state::susceptible) {
            this->m_circle.setOrigin(this->get_position());
            this->m_circle.setFillColor(sf::Color(66, 135, 245));
            this->m_circle.setOutlineThickness(0);
        } else if (this->m_state == state::infected) {
            this->m_circle.setOrigin(this->get_position());
            this->m_circle.setFillColor(sf::Color(235, 64, 52));
            this->m_circle.setOutlineThickness(infection_radius - 20.0f);
            this->m_circle.setOutlineColor(sf::Color(255, 255, 255, 20));
        } else  {
            this->m_circle.setOrigin(this->get_position());
            this->m_circle.setFillColor(sf::Color(80, 80, 80));
            this->m_circle.setOutlineThickness(0);
        }

        window.draw(this->m_circle);
    }
    
    void move(sf::Vector2f offset, float size) {
        this->m_position += offset;
        this->m_position = {std::clamp(this->m_position.x, -size, size), std::clamp(this->m_position.y, -size, size)};
    }

    bool is_in_radius(const std::unique_ptr<person>& person) {
        auto perpos = person->get_position();
        auto pos = this->get_position();
        float dist = std::sqrt(std::pow(perpos.x - pos.x, 2) + std::pow(perpos.y - pos.y, 2));

        return dist < infection_radius;
    }

    void set_position(sf::Vector2f pos) { this->m_position = pos; }
    sf::Vector2f get_position() const { return this->m_position; }

    void infect() {
        this->phase(state::infected);
        this->m_clock.restart();
    }

    void phase(state state) {
        if (this->m_state != state) {
            this->m_stats[this->m_state]--;
            this->m_state = state;
            this->m_stats[this->m_state]++;
        }
    }
    
    state get_state() const { return this->m_state; }
    
private:
    float m_speed;
    float m_direction = 0;
    sf::Clock m_clock;
    sf::Vector2f m_position;
    sf::CircleShape m_circle;
    std::uniform_real_distribution<float> random;
    std::uniform_real_distribution<float> infect_random;

    state m_state = state::susceptible;
    std::map<person::state, int>& m_stats;
};



class sir
{
public:
    sir(int count, float size): m_view({0, 0}, {size, size}), random(-size / 2, size / 2) {
        for (int i = 0; i < count; i++)
            this->m_everyone.push_back(std::make_unique<person>(sf::Vector2f(random(gen), random(gen)), speed, this->m_stats));

        this->m_stats[person::state::susceptible] = this->m_everyone.size();
        
        this->m_everyone[0]->infect();
        this->m_everyone[0]->set_position({0, 0});
    }

    void tick(float size) {
        for (auto& person: this->m_everyone)
            person->tick(this->m_everyone, size / 2);
    }
    
    void draw(sf::RenderWindow& window) {
        window.setView(this->m_view);
        for (auto& person: this->m_everyone)
            person->draw(window);
    }

    int get_count() const { return this->m_everyone.size(); }
    float get_size() const { return this->m_view.getSize().x; }

    std::map<person::state, int>& get_stats() { return this->m_stats; }
private:
    sf::View m_view;
    std::uniform_real_distribution<float> random;

    std::map<person::state, int> m_stats;
    
    // Actual simulation
    std::vector<std::unique_ptr<person>> m_everyone;
};



/*========================================================
 *ENTRY POINT
 */


int main(int argc, char *argv[])
{
    // Create an SFML window
    sf::RenderWindow window(sf::VideoMode(1440, 720), "SIR");
    
    // Create a simulation
    sir simulation(2500, 8000);
    std::cout << "created simulation with " << simulation.get_count() << " people." << std::endl;
    
    // Main loop
    sf::Font font;
    font.loadFromFile("/home/jlagarespo/.fonts/PxPlus_IBM_VGA8.ttf");
    sf::Text text("hello, world", font);
    text.setCharacterSize(200);
    text.setPosition(-simulation.get_size() / 2, -simulation.get_size() / 2);
    text.setFillColor(sf::Color::White);
    
    sf::Clock clock;
    int e = 0;
    int speed_limit = 60;
    while (window.isOpen()) {
        // Process events
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed:
                std::cout << "shutting down..." << std::endl;
                window.close();
                break;
            case sf::Event::KeyPressed:
                switch (event.key.code) {
                case sf::Keyboard::Period:
                    speed_limit += 20;
                    break;
                case sf::Keyboard::Comma:
                    speed_limit -= 20;
                    break;
                }
                break;
            }
        }

        // Tick simulation
        float t = clock.restart().asSeconds();
        simulation.tick(simulation.get_size());
        
        std::stringstream stream;
        stream << std::fixed << std::setprecision(4) << 1.0f / t << " tps " << t << " mspt" << std::endl << "epoch " << e << std::endl << "speed limit " << speed_limit << std::endl;
        
        stream << "susceptible: " << simulation.get_stats()[person::state::susceptible] << std::endl;
        stream << "infected: " << simulation.get_stats()[person::state::infected] << std::endl;
        stream << "removed: " << simulation.get_stats()[person::state::removed] << std::endl;
        
        text.setString(stream.str());
        
        // Draw stuff
        window.clear();
        simulation.draw(window);
        window.draw(text);
        
        // Update window
        window.setFramerateLimit(speed_limit);
        window.display();

        e++;
    }
    
    return 0;
}

/*% g++ sir.cc -o sir -std=c++2a -lsfml-graphics -lsfml-window -lsfml-system
 *% ./sir
 */