#pragma once

#include "ObjectKernel.hpp"

#include <array>
#include <cstdint>
#include <ghc/fs_fwd.hpp>
#include <string>
#include <mutex>

namespace tulip::text {
	struct GeneratorConfig {
		mutable std::mutex mutex;

		std::array<float, 2> position;
		std::array<float, 2> anchor;

		std::vector<ObjectKernel> kernels;

		ghc::filesystem::path fontPath;
		float fontSize = 36.0f;

		int32_t objectsPerGlyph = 50;
		float minScore = 10.0f;
		
		float negativeScore = -1.0f;
	};
}