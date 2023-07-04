#include <Generator.hpp>
#include <Geode/loader/Log.hpp>
#include <SFML/Graphics.hpp>
#include <fftw3.h>

#include <oneapi/tbb/parallel_for_each.h>

#include <random>
#include <algorithm>
#include <execution>
#include <locale>
#include <codecvt> 

using namespace geode::prelude;
using namespace tulip::text;

namespace tulip::text {
	struct GlyphData {
		sf::Glyph glyph;
		char32_t codepoint;
	};

	struct GlyphVector2D {
		std::vector<double> data;
		int32_t width;
		int32_t height;
		char32_t codepoint;
	};

	struct ConvolutionScore {
		double score = 0.0f;
		int32_t x = 0;
		int32_t y = 0;
		int32_t kernelId = 0;
	};

	struct FFTWData {
		double* input;
		double* kernelInput;
		fftw_complex* output;
		fftw_complex* kernelOutput;
		fftw_complex* multiplied;
		double* convolutionOutput;
		fftw_plan inputPlan;
		fftw_plan kernelPlan;
		fftw_plan convolutionPlan;
	};
}

class Generator::Impl {
public:

	std::vector<GlyphData> getUniqueGlyphs(
		std::u32string const& text, GeneratorConfig const& config, sf::Font& font
	);	

	std::map<char32_t, GlyphVector2D> getGlyphVectors(
		std::vector<GlyphData> const& glyphs, GeneratorConfig const& config, sf::Image const& fontImage
	);

	void addNegativeScores(
		std::map<char32_t, GlyphVector2D>& glyphVectors, GeneratorConfig const& config
	);

	std::vector<ConvolutionScore> getScoresForGlyph(
		GlyphVector2D& glyphVector, GeneratorConfig const& config
	);

	ConvolutionScore getConvolutionScore(
		GlyphVector2D const& glyphVector, ObjectKernel const& kernel, GeneratorConfig const& config, FFTWData& data
	) const;

	std::vector<CreatedObject> create(std::u32string const& text, GeneratorConfig const& config);
};

std::vector<GlyphData> Generator::Impl::getUniqueGlyphs(
	std::u32string const& text, GeneratorConfig const& config, sf::Font& font
) {
	std::vector<char32_t> chars;

	for (auto const& c : text) {
		if (std::find(chars.begin(), chars.end(), c) == chars.end()) {
			chars.push_back(c);
		}
	}

	std::vector<GlyphData> ret;

	for (auto const& c : chars) {
		ret.push_back({font.getGlyph(c, config.fontSize, false), c});
	}

	return ret;
}

std::map<char32_t, GlyphVector2D> Generator::Impl::getGlyphVectors(
	std::vector<GlyphData> const& glyphs, GeneratorConfig const& config, sf::Image const& fontImage
) {
	std::map<char32_t, GlyphVector2D> ret;

	for (auto const& [glyph, codepoint] : glyphs) {
		GlyphVector2D vec;
		vec.width = glyph.bounds.width;
		vec.height = glyph.bounds.height;
		vec.codepoint = codepoint;

		sf::Image glyphImage;
		glyphImage.create(vec.width, vec.height);
		glyphImage.copy(fontImage, 0, 0, glyph.textureRect);

		for (int32_t y = 0; y < vec.height; ++y) {
			for (int32_t x = 0; x < vec.width; ++x) {
				auto color = glyphImage.getPixel(x, y);
				vec.data.push_back(color.a / 255.0f);
			}
		}
		
		ret[codepoint] = vec;
	}

	return ret;
}

void Generator::Impl::addNegativeScores(
	std::map<char32_t, GlyphVector2D>& glyphVectors, GeneratorConfig const& config
) {
	for (auto& [codepoint, glyph] : glyphVectors) {
		for (auto& val : glyph.data) {
			if (val == 0.0f) {
				val = config.negativeScore;
			}
		}
	}
}

ConvolutionScore Generator::Impl::getConvolutionScore(
	GlyphVector2D const& glyphVector, ObjectKernel const& kernel, GeneratorConfig const& config, FFTWData& data
) const {
	ConvolutionScore ret;
	ret.score = 0.0f;

	auto width = std::max(glyphVector.width, kernel.width);
	auto height = std::max(glyphVector.height, kernel.height);

	auto input = data.input;
	auto kernelInput = data.kernelInput;
	auto output = data.output;
	auto kernelOutput = data.kernelOutput;
	auto multiplied = data.multiplied;
	auto convolutionOutput = data.convolutionOutput;
	auto inputPlan = data.inputPlan;
	auto kernelPlan = data.kernelPlan;
	auto convolutionPlan = data.convolutionPlan;

	// create fftw input
	for (int32_t i = 0; i < width * height; ++i) {
		input[i] = 0.0;
	}

	for (int32_t y = 0; y < glyphVector.height; ++y) {
		for (int32_t x = 0; x < glyphVector.width; ++x) {
			auto index = y * glyphVector.width + x;
			auto index2 = y * width + x;
			input[index2] = glyphVector.data[index];
		}
	}

	// create fftw kernel input
	for (int32_t i = 0; i < width * height; ++i) {
		kernelInput[i] = 0.0;
	}

	for (int32_t y = 0; y < kernel.height; ++y) {
		for (int32_t x = 0; x < kernel.width; ++x) {
			auto index = y * kernel.width + x;
			auto index2 = y * width + x;
			kernelInput[index2] = kernel.data[index];
		}
	}

	// execute fftw plans
	fftw_execute(inputPlan);
	fftw_execute(kernelPlan);

	// calculate convolution
	for (int32_t y = 0; y < height; ++y) {
		for (int32_t x = 0; x < width; ++x) {
			auto index = y * width + x;
			auto real = output[index][0];
			auto imag = output[index][1];

			auto kernelReal = kernelOutput[index][0];
			auto kernelImag = kernelOutput[index][1];

			multiplied[index][0] = real * kernelReal - imag * kernelImag;
			multiplied[index][1] = real * kernelImag + imag * kernelReal;
		}
	}

	fftw_execute(convolutionPlan);

	// find best score
	for (int32_t y = kernel.height - 1; y < height; ++y) {
		for (int32_t x = kernel.width - 1; x < width; ++x) {
			auto index = y * width + x;

			auto score = convolutionOutput[index];

			if (score > ret.score + 1) {
				ret.score = score;
				ret.x = x - kernel.width + 1;
				ret.y = y - kernel.height + 1;
			}
		}
	}

	return ret;
}

std::vector<ConvolutionScore> Generator::Impl::getScoresForGlyph(
	GlyphVector2D& glyphVector, GeneratorConfig const& config
) {
	std::vector<ConvolutionScore> ret;

	log::debug("Calculating scores for glyph");

	// create kernel ids
	std::vector<int32_t> kernelIds(config.kernels.size());
	std::iota(kernelIds.begin(), kernelIds.end(), 0);

	// for (int32_t y = 0; y < glyphVector.height; ++y) {
	// 	for (int32_t x = 0; x < glyphVector.width; ++x) {
	// 		auto index = y * glyphVector.width + x;
	// 		std::cout << (glyphVector.data[index] > 0.9 ? '#' : ' ') << ' ';
	// 	}
	// 	std::cout << '\n';
	// }

	log::debug("Calculating scores for glyph: {} kernels", kernelIds.size());

	std::vector<FFTWData> fftwData(config.kernels.size());
	for (int i = 0; i < config.kernels.size(); ++i) {
		auto& kernel = config.kernels[i];
		auto width = glyphVector.width;
		auto height = glyphVector.height;
		
		auto& data = fftwData[i];
		data.input = new (std::align_val_t(16)) double[width * height];
		data.kernelInput = new (std::align_val_t(16)) double[width * height];
		data.output = new (std::align_val_t(16)) fftw_complex[width * height];
		data.kernelOutput = new (std::align_val_t(16)) fftw_complex[width * height];
		data.multiplied = new (std::align_val_t(16)) fftw_complex[width * height];
		data.convolutionOutput = new (std::align_val_t(16)) double[width * height];
		data.inputPlan = fftw_plan_dft_r2c_2d(width, height, data.input, data.output,  FFTW_PATIENT);
		data.kernelPlan = fftw_plan_dft_r2c_2d(width, height, data.kernelInput, data.kernelOutput,  FFTW_PATIENT);
		data.convolutionPlan = fftw_plan_dft_c2r_2d(width, height, data.multiplied, data.convolutionOutput,  FFTW_PATIENT);
	}

	// repeat for every object added to glyph
	for (int32_t objectIndex = 0; objectIndex < config.objectsPerGlyph; ++objectIndex) {
		std::mutex mutex;
		ConvolutionScore bestScore;

		log::debug("Calculating scores for glyph: object {}", objectIndex);

		struct Body {
			using argument_type = int32_t;
			Generator::Impl const* impl;
			GlyphVector2D const& glyphVector;
			GeneratorConfig const& config;
			std::mutex& mutex;
			ConvolutionScore& bestScore;
			std::vector<FFTWData>& fftwData;

			Body(
				Generator::Impl const* impl,
				GlyphVector2D const& glyphVector,
				GeneratorConfig const& config,
				std::mutex& mutex,
				ConvolutionScore& bestScore,
				std::vector<FFTWData>& fftwData
			) :
				impl(impl),
				glyphVector(glyphVector),
				config(config),
				mutex(mutex),
				bestScore(bestScore),
				fftwData(fftwData)
			{}

			void operator()(int32_t id, oneapi::tbb::feeder<int32_t>& feeder) const {
				// calculate convolution score
				std::unique_lock<std::mutex> lock(mutex);
				auto& kernel = config.kernels[id];
				auto& data = fftwData[id];
				lock.unlock();

				if (kernel.width > glyphVector.width || kernel.height > glyphVector.height) {
					return;
				}

				auto score = impl->getConvolutionScore(glyphVector, kernel, config, data);
				score.kernelId = id;

				// if better than best, update best
				lock.lock();
				if (score.score >= bestScore.score) {
					bestScore = score;
				}
				lock.unlock();
			}
		};

		log::debug("Calculating scores for glyph: object {}: {} kernels", objectIndex, kernelIds.size());

		auto body = Body(
			this,
			glyphVector,
			config,
			mutex,
			bestScore,
			fftwData
		);
		// for each convolution id
		oneapi::tbb::parallel_for_each(kernelIds.begin(), kernelIds.end(), body);

		// break;

		if (bestScore.score < config.minScore) {
			break;
		}

		log::debug("Applying convolution {} to glyph {}, {}", bestScore.kernelId, bestScore.x, bestScore.y);

		// apply the best convolution
		auto& kernel = config.kernels[bestScore.kernelId];
		
		for (int32_t y = 0; y < kernel.height; ++y) {
			for (int32_t x = 0; x < kernel.width; ++x) {
				auto index = y * kernel.width + x;
				auto index2 = (y + bestScore.y) * glyphVector.width + (x + bestScore.x);

				// if kernel is positive and glyph is positive, subtract kernel from glyph
				if (kernel.data[index] > 0.0f && glyphVector.data[index2] > 0.0f) {
					// square the kernel because handling transparency is hard
					glyphVector.data[index2] -= kernel.data[index] * kernel.data[index];
					if (glyphVector.data[index2] < 0.0f) {
						glyphVector.data[index2] = 0.0f;
					}
				}
			}
		}

		// for (int32_t y = 0; y < glyphVector.height; ++y) {
		// 	for (int32_t x = 0; x < glyphVector.width; ++x) {
		// 		auto index = y * glyphVector.width + x;
		// 		auto convolutionOutput = fftwData[0].convolutionOutput;;
		// 		std::cout << convolutionOutput[index] << '\t';
		// 	}
		// 	std::cout << '\n';
		// }

		// for (int32_t y = 0; y < glyphVector.height; ++y) {
		// 	for (int32_t x = 0; x < glyphVector.width; ++x) {
		// 		auto index = y * glyphVector.width + x;
		// 		auto kernelInput = fftwData[0].kernelInput;;
		// 		std::cout << kernelInput[index] << '\t';
		// 	}
		// 	std::cout << '\n';
		// }

		log::debug("Glyph score: {}", bestScore.score);

		// add the score to the list
		ret.push_back(bestScore);

		// for (int32_t y = 0; y < glyphVector.height; ++y) {
		// 	for (int32_t x = 0; x < glyphVector.width; ++x) {
		// 		auto index = y * glyphVector.width + x;
		// 		std::cout << (glyphVector.data[index] > 0.9 ? '#' : ' ') << ' ';
		// 	}
		// 	std::cout << '\n';
		// }
	}

	for (auto& data : fftwData) {
		delete[] data.input;
		delete[] data.kernelInput;
		delete[] data.output;
		delete[] data.kernelOutput;
		fftw_destroy_plan(data.inputPlan);
		fftw_destroy_plan(data.kernelPlan);
		fftw_destroy_plan(data.convolutionPlan);
	}

	log::debug("Calculated scores for glyph");

	return ret;
}

std::vector<CreatedObject> Generator::Impl::create(
	std::u32string const& text, GeneratorConfig const& config
) {
	log::debug("Creating text");

	// get all unique glyphs in text
	sf::Font font;
	font.loadFromFile(config.fontPath);
	auto glyphs = this->getUniqueGlyphs(text, config, font);

	log::debug("Found {} unique glyphs", glyphs.size());

	// get matrix representations for each
	sf::Image fontImage = font.getTexture(config.fontSize).copyToImage();
	auto glyphVectors = this->getGlyphVectors(glyphs, config, fontImage);

	log::debug("Created {} glyph vectors", glyphVectors.size());

	// add the negative scores to blank spaces
	this->addNegativeScores(glyphVectors, config);

	std::map<char32_t, std::vector<ConvolutionScore>> scoreMap;

	log::debug("Calculating convolution scores");

	// for each glyph
	for (auto& [codepoint, glyphVector] : glyphVectors) {
		log::debug("Calculating convolution scores");

		// get the scores for the glyph
		auto scores = this->getScoresForGlyph(glyphVector, config);

		log::debug("Calculated {} convolution scores", scores.size());

		// add the scores to the map
		scoreMap[codepoint] = scores;
	}

	log::debug("Creating objects");

	// create the text 
	sf::Text textObject;
	textObject.setFont(font);
	textObject.setCharacterSize(config.fontSize);
	textObject.setFillColor(sf::Color::White);
	textObject.setString(sf::String::fromUtf32(text.begin(), text.end()));

	log::debug("Created text object");

	// create the objects
	std::vector<CreatedObject> ret;
	for (uint32_t i = 0; i < text.size(); ++i) {
		auto c = text[i];
		auto& scores = scoreMap[c];

		auto cursor = textObject.findCharacterPos(i);

		for (auto& score : scores) {
			auto& kernel = config.kernels[score.kernelId];
			// create the object
			CreatedObject object;
			// TODO: config.anchor
			object.x = config.positionX + kernel.offsetX + double(cursor.x + score.x) / 2; // (60x60)
			object.y = config.positionY + kernel.offsetY - double(cursor.y + score.y) / 2;
			object.objectId = kernel.objectId;
			object.scale = kernel.scale;
			object.rotation = kernel.rotation;

			// add the object to the list
			ret.push_back(object);
		}
	}

	log::debug("Created {} objects", ret.size());

	// return the objects
	return ret;
}

Generator::Generator() :
	m_impl(new Impl) {}

Generator::~Generator() {}

Generator* Generator::get() {
	static Generator s_ret;
	return &s_ret;
}

std::vector<CreatedObject> Generator::create(
	std::u32string const& text, GeneratorConfig const& config
) {
	return m_impl->create(text, config);
}