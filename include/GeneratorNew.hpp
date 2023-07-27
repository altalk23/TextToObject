#pragma once

#include "MatrixOperations.hpp"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

namespace tulip::text {

	class GeneratorNew {
		class Impl;
		std::unique_ptr<Impl> m_impl;

	public:
		GeneratorNew();
		~GeneratorNew();

		void init(char32_t glyph);
        void addKernel(sf::Sprite& kernel, double scale);
        void step(int steps = 1);

        sf::Image const& getGlyph() const;
        sf::Image const& getEdge() const;
        sf::Image const& getFilled() const;
        sf::Image const& getRemoved() const;
        sf::Image const& getKernel() const;
        sf::Image const& getKernel2(int index) const;
	};
}