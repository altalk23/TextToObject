#pragma once

namespace tulip::text {
	struct ObjectKernel {
		std::vector<double> data;
		int32_t width;
		int32_t height;
		double offsetX;
		double offsetY;

		int32_t objectId;
		double scale;
		double rotation;
	};
}