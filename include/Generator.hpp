#pragma once

#include "CreatedObject.hpp"
#include "GeneratorConfig.hpp"

#include <memory>
#include <vector>

namespace tulip::text {

	class Generator {
		class Impl;
		std::unique_ptr<Impl> m_impl;

	public:
		Generator();
		~Generator();
		static Generator* get();

		std::vector<CreatedObject> create(std::u32string const& text, GeneratorConfig const& config);
	};
}