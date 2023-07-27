#include <GeneratorNew.hpp>

#include <SFML/Graphics.hpp>

using namespace tulip::text;

GeneratorNew::GeneratorNew() : m_impl(std::make_unique<Impl>()) {}

GeneratorNew::~GeneratorNew() = default;

class GeneratorNew::Impl {
public:
    std::vector<sf::Image> m_kernels;

    int m_width;
    int m_height;

    sf::Image m_glyphImage;
    sf::Image m_edgeImage;
    sf::Image m_filledImage;
    sf::Image m_removedImage;
    sf::Image m_kernelImage;

    double bestScore = 0;
    int bestIndex = 0;
    int bestX = 0;
    int bestY = 0;

    Matrix<double> m_input;
    Matrix<double> m_kernel;
    Matrix<double> m_output;
    Convolution m_convolution;

    Impl() : m_input(300, 300), m_kernel(300, 300), m_output(300, 300), m_convolution(m_input, m_kernel, m_output) {}

    void init(char32_t glyph);
    void addKernel(sf::Sprite const& kernel);
    void step(int steps);

    sf::Sprite getGlyph() const;
    sf::Sprite getEdge() const;
    sf::Sprite getFilled() const;
    sf::Sprite getRemoved() const;
    sf::Sprite getKernel() const;
};

void GeneratorNew::Impl::init(char32_t glyph) {
    sf::Font font;
    font.loadFromFile("/Users/student/Desktop/NotoSansJP-Regular.ttf");

    auto glyph2 = font.getGlyph(glyph, 150, false);

    sf::Image fontImage = font.getTexture(150).copyToImage();

    m_width = glyph2.textureRect.width;
    m_height = glyph2.textureRect.height;

    m_glyphImage.create(m_width, m_height, sf::Color::White);
    m_glyphImage.copy(fontImage, 0, 0, glyph2.textureRect);

    m_edgeImage.create(m_width, m_height, sf::Color::Black);

    m_filledImage.create(m_width, m_height, sf::Color::Black);

    m_kernelImage.create(m_width, m_height, sf::Color::Black);

    m_removedImage.create(m_width, m_height, sf::Color::White);
    m_removedImage.copy(fontImage, 0, 0, glyph2.textureRect);
}

void GeneratorNew::Impl::addKernel(sf::Sprite const& kernel) {
    int width = sprite.getGlobalBounds.width; 
    int height = sprite.getGlobalBounds.height;

    sf::RenderTexture renderTexture;
    renderTexture.create( width, height );
    renderTexture.clear( sf::Color::Black );
    renderTexture.draw( sprite );
    renderTexture.display();

    sf::Image image = renderTexture.getTexture().copyToImage();

    m_kernels.push_back(image);
}

void GeneratorNew::Impl::step(int steps) {
    for (int w = 0; w < steps; ++w) {
        bestScore = 0;

        m_input.zero();
        for (int x = 0; x < m_width; ++x) {
            for (int y = 0; y < m_height; ++y) {
                auto color = m_removedImage.getPixel(x, y);
                if (color.r > 0) {
                    m_input(x, y) = 1;
                }
            }
        }

        m_kernel.zero();
        m_kernel(1, 0) = -1;
        m_kernel(0, 1) = -1;
        m_kernel(1, 1) = 4;
        m_kernel(2, 1) = -1;
        m_kernel(1, 2) = -1;

        m_convolution.execute();

        for (int x = 0; x < m_width; ++x) {
            for (int y = 0; y < m_height; ++y) {
                if (m_output(x+1, y+1) > 0.5) {
                    m_edgeImage.setPixel(x, y, sf::Color::White);
                } else {
                    m_edgeImage.setPixel(x, y, sf::Color::Black);
                }
            }
        }

        for (int i = 0; i < m_kernels.size(); ++i) {
            auto& kernel = m_kernels[i];
            auto kernelWidth = kernel.getSize().x;
            auto kernelHeight = kernel.getSize().y;
            
            m_input.fill(-10);
            for (int x = 0; x < m_width; ++x) {
                for (int y = 0; y < m_height; ++y) {
                    auto colorE = m_edgeImage.getPixel(x, y);
                    auto colorR = m_removedImage.getPixel(x, y);

                    if (colorE.r > 0) {
                        m_input(x, y) = 1;
                    }
                    else if (colorR.r > 0) {
                        m_input(x, y) = 0;
                    }
                }
            }

            m_kernel.zero();
            for (int x = 0; x < kernelWidth; ++x) {
                for (int y = 0; y < kernelHeight; ++y) {
                    auto color = kernel.getPixel(x, y);
                    if (color.r > 0) {
                        m_kernel(x, y) = 1;
                    }
                }
            }

            m_convolution.execute();

            double score = 0;
            for (int x = 0; x < m_width - kernelWidth + 1; ++x) {
                for (int y = 0; y < m_height - kernelHeight + 1; ++y) {
                    auto current = m_output(x + kernelWidth - 1, y + kernelHeight - 1);

                    if (current + 0.1 > bestScore) {
                        bestScore = current;
                        bestKernel = i;
                        bestX = x;
                        bestY = y;
                    }
                }
            }
        }

        auto& kernel = m_kernels[bestKernel];
        auto kernelWidth = kernel.getSize().x;
        auto kernelHeight = kernel.getSize().y;

        for (int x = 0; x < m_width; ++x) {
            for (int y = 0; y < m_height; ++y) {
                m_kernelImage.setPixel(x, y, sf::Color::Black);
            }
        }

        for (int x = 0; x < kernelWidth; ++x) {
            for (int y = 0; y < kernelHeight; ++y) {
                auto color = kernel.getPixel(x, y);
                if (color.r > 0) {
                    m_kernelImage.setPixel(bestX + x, bestY + y, sf::Color::White);
                    m_removedImage.setPixel(bestX + x, bestY + y, sf::Color::Black);
                    m_filledImage.setPixel(bestX + x, bestY + y, sf::Color::White);
                }
            }
        }

    }
}

sf::Sprite GeneratorNew::Impl::getGlyph() const {
    sf::Texture texture;
    texture.loadFromImage(m_glyphImage);

    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setPosition(100, 100);
    sprite.setScale(4, 4);
    return sprite;
}
sf::Sprite GeneratorNew::Impl::getEdge() const {
    sf::Texture texture;
    texture.loadFromImage(m_edgeImage);

    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setPosition(100, 100);
    sprite.setScale(4, 4);
    return sprite;
}
sf::Sprite GeneratorNew::Impl::getFilled() const {
    sf::Texture texture;
    texture.loadFromImage(m_filledImage);

    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setPosition(100, 100);
    sprite.setScale(4, 4);
    return sprite;
}
sf::Sprite GeneratorNew::Impl::getRemoved() const {
    sf::Texture texture;
    texture.loadFromImage(m_removedImage);

    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setPosition(100, 100);
    sprite.setScale(4, 4);
    return sprite;
}
sf::Sprite GeneratorNew::Impl::getKernel() const {
    sf::Texture texture;
    texture.loadFromImage(m_kernelImage);

    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setPosition(100, 100);
    sprite.setScale(4, 4);
    return sprite;
}

void GeneratorNew::init(char32_t glyph) {
    m_impl->init(glyph);
}

void GeneratorNew::addKernel(sf::Sprite const& kernel) {
    m_impl->addKernel(kernel);
}

void GeneratorNew::step(int steps) {
    m_impl->step(steps);
}

sf::Sprite GeneratorNew::getGlyph() const {
    return m_impl->getGlyph();
}

sf::Sprite GeneratorNew::getEdge() const {
    return m_impl->getEdge();
}

sf::Sprite GeneratorNew::getFilled() const {
    return m_impl->getFilled();
}

sf::Sprite GeneratorNew::getRemoved() const {
    return m_impl->getRemoved();
}

sf::Sprite GeneratorNew::getKernel() const {
    return m_impl->getKernel();
}