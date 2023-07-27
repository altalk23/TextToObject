#include <MatrixOperations.hpp>
#include <iostream>
#include <fftw3.h>
#include <SFML/Graphics.hpp>
#include <GeneratorNew.hpp>

using namespace tulip::text;

std::vector<sf::Sprite> sprites;

int main() {
    sf::RenderWindow window;
    window.create(sf::VideoMode(800, 800), "My window");

    GeneratorNew generator;
    generator.init(U'〇');

    sf::Image kernelImage;
    kernelImage.create(60, 60, sf::Color::White);
    sf::Texture kernelTexture;
    kernelTexture.loadFromImage(kernelImage);

    for (int size = 8; size < 20; size += 1) {
        sf::Sprite kernelSprite;
        kernelSprite.setTexture(kernelTexture);
        kernelSprite.setOrigin(30, 30);

        kernelSprite.setScale(size / 60.0, size / 60.0);
        kernelSprite.setPosition(size, size);
        generator.addKernel(kernelSprite, size / 60.0);

        if (size % 2 == 1) continue;

        for (double rotation = 15; rotation < 90; rotation += 15) {
            kernelSprite.setRotation(rotation);
            generator.addKernel(kernelSprite, size / 60.0);
        }
    }

    for (int size = 8; size < 20; size += 1) {
        sf::Sprite kernelSprite;
        kernelSprite.setTexture(kernelTexture);
        kernelSprite.setOrigin(30, 30);
        kernelSprite.setPosition(3*size, 3*size);

        kernelSprite.setScale(size / 60.0, size / 20.0);
        generator.addKernel(kernelSprite, size / 20.0);

        kernelSprite.setScale(size / 20.0, size / 60.0);
        generator.addKernel(kernelSprite, size / 20.0);

        if (size % 2 == 1) continue;

        for (double rotation = 15; rotation < 180; rotation += 15) {
            if (rotation == 90) continue;
            kernelSprite.setRotation(rotation);
            generator.addKernel(kernelSprite, size / 20.0);
        }
    }

    sf::Font font;
    font.loadFromFile("/Users/student/Desktop/NotoSansJP-Regular.ttf");    

    window.setKeyRepeatEnabled(false);
    // run the program as long as the window is open
    size_t current = 0;
    size_t step = 0;
    bool update = true;
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Right) {
                    current = (current + 1) % 5;
                    std::cout << current << std::endl;
                }
                if (event.key.code == sf::Keyboard::Left) {
                    current = (current + 4) % 5;
                    std::cout << current << std::endl;
                }
                if (event.key.code == sf::Keyboard::Space) {
                    generator.step();
                    ++step;
                }
                update = true;
            }
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed) {
                window.close();
                return 0;
            }
        }

        if (!update) {
            continue;
        }

        sf::Image const* image;
        switch (current) {
            case 0:
                image = &generator.getGlyph();
                break;
            case 1:
                image = &generator.getEdge();
                break;
            case 2:
                image = &generator.getFilled();
                break;
            case 3:
                image = &generator.getRemoved();
                break;
            case 4:
                image = &generator.getKernel();
                break;
        }
        // image = &generator.getKernel2(current);

        // Create a text
        sf::Text text("current: " + std::to_string(current) + " step: " + std::to_string(step), font);
        text.setCharacterSize(30);
        text.setFillColor(sf::Color::White);
        text.setPosition(50, 50);
        
        

        sf::Texture texture;
        texture.loadFromImage(*image);

        sf::Sprite sprite;
        sprite.setTexture(texture);
        sprite.setScale(3, 3);
        sprite.setPosition(100, 100);

        window.clear();
        window.draw(sprite);
        window.draw(text);
        window.display();
    }

    return 0;
}