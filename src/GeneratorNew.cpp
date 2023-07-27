#include <GeneratorNew.hpp>
#include <iostream>
#include <SFML/Graphics.hpp>

using namespace tulip::text;

GeneratorNew::GeneratorNew() : m_impl(std::make_unique<Impl>()) {}

GeneratorNew::~GeneratorNew() = default;

class GeneratorNew::Impl {
public:
    std::vector<sf::Image> m_kernels;
    std::vector<bool> m_offset;

    int m_width;
    int m_height;

    sf::Image m_glyphImage;
    sf::Image m_edgeImage;
    sf::Image m_filledImage;
    sf::Image m_removedImage;
    sf::Image m_kernelImage;

    double bestScore = 0;
    int bestKernel = 0;
    int bestX = 0;
    int bestY = 0;

    Matrix<double> m_input;
    Matrix<double> m_kernel;
    Matrix<double> m_output;
    Convolution m_convolution;

    Impl() : m_input(400, 400), m_kernel(400, 400), m_output(400, 400), m_convolution(m_input, m_kernel, m_output) {}

    void init(char32_t glyph);
    void addKernel(sf::Sprite& kernel, double scale);
    void step(int steps);

    sf::Image const& getGlyph() const;
    sf::Image const& getEdge() const;
    sf::Image const& getFilled() const;
    sf::Image const& getRemoved() const;
    sf::Image const& getKernel() const;
};

sf::Image const& GeneratorNew::getKernel2(int index) const {
    return m_impl->m_kernels[index];
}

void GeneratorNew::Impl::init(char32_t glyph) {
    sf::Font font;
    font.loadFromFile("/Users/student/Desktop/NotoSansJP-Regular.ttf");

    auto glyph2 = font.getGlyph(glyph, 200, false);

    sf::Image fontImage = font.getTexture(200).copyToImage();

    m_width = glyph2.textureRect.width;
    m_height = glyph2.textureRect.height;

    // std::cout << m_width << " " << m_height << std::endl;

    m_glyphImage.create(m_width, m_height, sf::Color::Black);
    m_glyphImage.copy(fontImage, 0, 0, glyph2.textureRect);
    for (int x = 0; x < m_width; ++x) {
        for (int y = 0; y < m_height; ++y) {
            auto color = m_glyphImage.getPixel(x, y);
            if (color.a > 127) {
                m_glyphImage.setPixel(x, y, sf::Color::White);
            }
            else {
                m_glyphImage.setPixel(x, y, sf::Color::Black);
            }
        }
    }

    m_edgeImage.create(m_width, m_height, sf::Color::Black);

    m_filledImage.create(m_width, m_height, sf::Color::Black);

    m_kernelImage.create(m_width, m_height, sf::Color::Black);

    m_removedImage.create(m_width, m_height, sf::Color::Black);
    m_removedImage.copy(m_glyphImage, 0, 0, { 0, 0, m_width, m_height });
}

void GeneratorNew::Impl::addKernel(sf::Sprite& kernel, double scale) {
    int width = std::round(120 * scale); 
    int height = std::round(120 * scale);

    sf::RenderTexture renderTexture;
    renderTexture.create( width, height );
    renderTexture.clear( sf::Color::Black );
    renderTexture.draw( kernel );
    renderTexture.display();

    sf::Image image = renderTexture.getTexture().copyToImage();

    m_kernels.push_back(image);
    m_offset.push_back(int(std::round(scale * 60)) % 2 == 1);
}

void GeneratorNew::Impl::step(int steps) {
    for (int w = 0; w < steps; ++w) {
        bestScore = 0;

        m_input.zero();
        for (int x = 0; x < m_width; ++x) {
            for (int y = 0; y < m_height; ++y) {
                auto color = m_removedImage.getPixel(x, y);
                if (color.r > 127) {
                    m_input(x, y) = 1;
                }
            }
        }

        m_kernel.zero();
        for (int x = 1; x <= 3; ++x) {
            for (int y = 1; y <= 3; ++y) {
                m_kernel(x, y) = -1;
            }
        }
        m_kernel(2, 0) = -1;
        m_kernel(0, 2) = -1;
        m_kernel(2, 2) = 12;
        m_kernel(4, 2) = -1;
        m_kernel(2, 4) = -1;

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
            
            m_input.fill(-4);
            for (int x = 0; x < m_width; ++x) {
                for (int y = 0; y < m_height; ++y) {
                    auto colorE = m_edgeImage.getPixel(x, y);
                    auto colorG = m_glyphImage.getPixel(x, y);
                    auto colorF = m_filledImage.getPixel(x, y);

                    if (colorE.r > 0) {
                        m_input(x, y) = 1;
                    }
                    else if (colorG.r > 0) {
                        if (colorF.r > 0) {
                            m_input(x, y) = 0; // -0.02;
                        }
                        else {
                            m_input(x, y) = 0;
                        }
                    }

                }
            }

            m_kernel.zero();
            for (int x = 0; x < kernelWidth; ++x) {
                for (int y = 0; y < kernelHeight; ++y) {
                    auto color = kernel.getPixel(x, y);
                    if (color.r > 127) {
                        m_kernel(x, y) = 1;
                    }
                }
            }

            m_convolution.execute();

            double score = 0;
            for (int x = 0; x < m_width + kernelWidth; ++x) {
                for (int y = 0; y < m_height + kernelHeight; ++y) {
                    auto current = m_output(x, y);

                    if (current + 0.1 > bestScore) {
                        bestScore = current;
                        bestKernel = i;
                        bestX = x - kernelWidth + 1 + m_offset[i];
                        bestY = y - kernelHeight + 1 - m_offset[i];
                    }
                }
            }
        }

        std::cout << bestKernel << " " << bestScore << " " << bestX << " " << bestY << std::endl;

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
                if (bestX + x < 0 || bestX + x >= m_width || bestY + y < 0 || bestY + y >= m_height) {
                    continue;
                }
                if (color.r > 127) {
                    m_kernelImage.setPixel(bestX + x, bestY + y, sf::Color::White);
                    m_removedImage.setPixel(bestX + x, bestY + y, sf::Color::Black);
                    m_filledImage.setPixel(bestX + x, bestY + y, sf::Color::White);
                }
            }
        }

    }
}

sf::Image const& GeneratorNew::Impl::getGlyph() const {
    return m_glyphImage;
}
sf::Image const& GeneratorNew::Impl::getEdge() const {
    return m_edgeImage;
}
sf::Image const& GeneratorNew::Impl::getFilled() const {
    return m_filledImage;
}
sf::Image const& GeneratorNew::Impl::getRemoved() const {
    return m_removedImage;
}
sf::Image const& GeneratorNew::Impl::getKernel() const {
    return m_kernelImage;
}

void GeneratorNew::init(char32_t glyph) {
    m_impl->init(glyph);
}

void GeneratorNew::addKernel(sf::Sprite& kernel, double scale) {
    m_impl->addKernel(kernel, scale);
}

void GeneratorNew::step(int steps) {
    m_impl->step(steps);
}

sf::Image const& GeneratorNew::getGlyph() const {
    return m_impl->getGlyph();
}

sf::Image const& GeneratorNew::getEdge() const {
    return m_impl->getEdge();
}

sf::Image const& GeneratorNew::getFilled() const {
    return m_impl->getFilled();
}

sf::Image const& GeneratorNew::getRemoved() const {
    return m_impl->getRemoved();
}

sf::Image const& GeneratorNew::getKernel() const {
    return m_impl->getKernel();
}