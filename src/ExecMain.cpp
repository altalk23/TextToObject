#include <MatrixOperations.hpp>
#include <iostream>
#include <fftw3.h>
#include <SFML/Graphics.hpp>

using namespace tulip::text;

std::vector<sf::Sprite> sprites;

void addSprite(sf::Texture const& texture) {
    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setPosition(100, 100);
    sprite.setScale(4, 4);
    sprites.push_back(sprite);
}

int main() {
    sf::RenderWindow window;
    window.create(sf::VideoMode(800, 800), "My window");

    auto constexpr width = 300;
    auto constexpr height = 300;

    sf::Font font;
	font.loadFromFile("/Users/student/Desktop/NotoSansJP-Regular.ttf");

    auto glyph = font.getGlyph(U'ア', 150, false);

    sf::Image fontImage = font.getTexture(150).copyToImage();

    sf::Image glyphImage;
    glyphImage.create(glyph.textureRect.width, glyph.textureRect.height, sf::Color::White);
    glyphImage.copy(fontImage, 0, 0, glyph.textureRect);

    sf::Texture glyphTexture;
    glyphTexture.loadFromImage(glyphImage);
    addSprite(glyphTexture);

    sf::Image glyphImage2;
    glyphImage2.create(glyph.textureRect.width, glyph.textureRect.height, sf::Color::White);
    for (int x = 0; x < glyphImage.getSize().x; ++x) {
        for (int y = 0; y < glyphImage.getSize().y; ++y) {
            auto color = glyphImage.getPixel(x, y);
            if (color.a > 127) {
                glyphImage2.setPixel(x, y, sf::Color::White);
            }
            else {
                glyphImage2.setPixel(x, y, sf::Color::Black);
            }
        }
    }

    sf::Texture glyphTexture2;
    glyphTexture2.loadFromImage(glyphImage2);
    addSprite(glyphTexture2);

    Matrix<double> input(width, height);
    Matrix<double> kernel(width, height);
    Matrix<double> output(width, height);

    Convolution convolution(input, kernel, output);

    for (int x = 0; x < glyphImage2.getSize().x; ++x) {
        for (int y = 0; y < glyphImage2.getSize().y; ++y) {
            input(x, y) = glyphImage2.getPixel(x, y).r / 255.0;
        }
    }

    kernel(0, 1) = -1;
    kernel(1, 0) = -1;
    kernel(1, 2) = -1;
    kernel(2, 1) = -1;
    kernel(1, 1) = 4;

    convolution.execute();

    sf::Image outputImage;
    outputImage.create(glyph.textureRect.width, glyph.textureRect.height, sf::Color::White);
    for (int x = 0; x < outputImage.getSize().x; ++x) {
        for (int y = 0; y < outputImage.getSize().y; ++y) {
            auto color = sf::Color::White;
            color.r = std::min(255.0, output(x+1, y+1) * 255);
            color.g = std::min(255.0, output(x+1, y+1) * 255);
            color.b = std::min(255.0, output(x+1, y+1) * 255);
            outputImage.setPixel(x, y, color);
        }
    }

    sf::Texture outputTexture;
    outputTexture.loadFromImage(outputImage);
    addSprite(outputTexture);

    for (int x = 0; x < input.width; ++x) {
        for (int y = 0; y < input.height; ++y) {
            input(x, y) = -10;
        }
    }
    for (int x = 0; x < glyphImage2.getSize().x; ++x) {
        for (int y = 0; y < glyphImage2.getSize().y; ++y) {
            if (glyphImage2.getPixel(x, y).r != 0) {
                input(x, y) = 0;
            }
            if (outputImage.getPixel(x, y).r != 0) {
                input(x, y) = 1;
            }
        }
    }

    for (int x = 0; x < 14; ++x) {
        for (int y = 0; y < 14; ++y) {
            kernel(x, y) = 1;
        }
    }

    convolution.execute();

    sf::Image outputImage2;
    outputImage2.create(glyph.textureRect.width, glyph.textureRect.height, sf::Color::White);
    for (int x = 0; x < outputImage2.getSize().x; ++x) {
        for (int y = 0; y < outputImage2.getSize().y; ++y) {
            auto color = sf::Color::White;
            color.r = std::max(0.0, std::min(255.0, output(x+12, y+12) * 3));
            color.g = std::max(0.0, std::min(255.0, output(x+12, y+12) * 3));
            color.b = std::max(0.0, std::min(255.0, output(x+12, y+12) * 3));
            outputImage2.setPixel(x, y, color);
        }
    }

    sf::Texture outputTexture2;
    outputTexture2.loadFromImage(outputImage2);
    addSprite(outputTexture2);


    window.setKeyRepeatEnabled(false);
    // run the program as long as the window is open
    size_t current = 0;
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    current = (current + 1) % sprites.size();
                    std::cout << "current: " << current << std::endl;
                }
            }
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(sprites[current]);
        window.display();
    }

    return 0;
}