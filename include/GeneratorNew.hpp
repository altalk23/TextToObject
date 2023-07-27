#pragma once

#include "GeneratorConfig.hpp"
#include "MatrixOperations.hpp"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

namespace tulip::text {

	class GeneratorNew {
		class Impl;
		std::unique_ptr<Impl> m_impl;

	public:
		Generator();
		~Generator();

		void init(char32_t glyph);
        void addKernel(sf::Sprite const& kernel);
        void step(int steps = 1);

        sf::Sprite getGlyph() const;
        sf::Sprite getEdge() const;
        sf::Sprite getFilled() const;
        sf::Sprite getRemoved() const;
        sf::Sprite getKernel() const;
	};
}