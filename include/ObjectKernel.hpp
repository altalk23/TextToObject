#pragma once

namespace tulip::text {
	struct ObjectKernel {
		std::vector<float> data;
		int32_t width;
		int32_t height;

		int32_t objectId;
		float scale;
		float rotation;
	};
}