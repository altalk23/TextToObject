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

		double positionX;
		double positionY;
		double anchorX;
		double anchorY;

		std::vector<ObjectKernel> kernels;

		ghc::filesystem::path fontPath;
		double fontSize = 36.0f;

		int32_t objectsPerGlyph = 50;
		double minScore = 10.0f;
		
		double negativeScore = -1.0f;
	};
}