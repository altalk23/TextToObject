#include <Generator.hpp>
#include <SFML/Graphics.hpp>
#include <fftw3.h>

#include <oneapi/tbb/parallel_for_each.h>

#include <random>
#include <algorithm>
#include <execution>

using namespace tulip::text;

namespace tulip::text {
	struct GlyphData {
		sf::Glyph glyph;
		char32_t codepoint;
	};

	struct GlyphVector2D {
		std::vector<float> data;
		int32_t width;
		int32_t height;
		char32_t codepoint;
	};

	struct ConvolutionScore {
		float score = 0.0f;
		int32_t x = 0;
		int32_t y = 0;
		int32_t kernelId = 0;
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
		GlyphVector2D glyphVector, GeneratorConfig const& config
	);

	ConvolutionScore getConvolutionScore(
		GlyphVector2D const& glyphVector, ObjectKernel const& kernel, GeneratorConfig const& config, std::mutex& mutex
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
	GlyphVector2D const& glyphVector, ObjectKernel const& kernel, GeneratorConfig const& config, std::mutex& mutex
) const {
	ConvolutionScore ret;
	ret.score = 0.0f;

	auto maxWidth = std::max(glyphVector.width, kernel.width);
	auto maxHeight = std::max(glyphVector.height, kernel.height);

	auto convolutionWidth = maxWidth - kernel.width + 1;
	auto convolutionHeight = maxHeight - kernel.height + 1;

	std::unique_lock<std::mutex> lock(mutex);
	// allocate fftw arrays
	auto input = fftwf_alloc_complex(maxWidth * maxHeight);
	auto kernelInput = fftwf_alloc_complex(maxWidth * maxHeight);
	auto output = fftwf_alloc_complex(maxWidth * maxHeight);
	auto kernelOutput = fftwf_alloc_complex(maxWidth * maxHeight);

	// create fftw plans
	auto inputPlan = fftwf_plan_dft_2d(maxWidth, maxHeight, input, output, FFTW_FORWARD, FFTW_ESTIMATE);
	auto kernelPlan = fftwf_plan_dft_2d(maxWidth, maxHeight, kernelInput, kernelOutput, FFTW_FORWARD, FFTW_ESTIMATE);
	auto convolutionPlan = fftwf_plan_dft_2d(maxWidth, maxHeight, output, input, FFTW_BACKWARD, FFTW_ESTIMATE);

	lock.unlock();

	// create fftw input
	for (int32_t i = 0; i < maxWidth * maxHeight; ++i) {
		input[i][0] = 0.0f;
		input[i][1] = 0.0f;
	}

	for (int32_t y = 0; y < glyphVector.height; ++y) {
		for (int32_t x = 0; x < glyphVector.width; ++x) {
			auto index = y * glyphVector.width + x;
			auto index2 = y * maxWidth + x;
			input[index2][0] = glyphVector.data[index];
		}
	}

	// create fftw kernel input
	for (int32_t i = 0; i < maxWidth * maxHeight; ++i) {
		kernelInput[i][0] = 0.0f;
		kernelInput[i][1] = 0.0f;
	}

	for (int32_t y = 0; y < kernel.height; ++y) {
		for (int32_t x = 0; x < kernel.width; ++x) {
			auto index = y * kernel.width + x;
			auto index2 = y * maxWidth + x;
			kernelInput[index2][0] = kernel.data[index];
		}
	}

	// execute fftw plans
	fftwf_execute(inputPlan);
	fftwf_execute(kernelPlan);

	// calculate convolution
	for (int32_t y = 0; y < maxHeight; ++y) {
		for (int32_t x = 0; x < maxWidth; ++x) {
			auto index = y * maxWidth + x;
			auto real = output[index][0];
			auto imag = output[index][1];

			auto kernelReal = kernelOutput[index][0];
			auto kernelImag = kernelOutput[index][1];

			output[index][0] = real * kernelReal - imag * kernelImag;
			output[index][1] = real * kernelImag + imag * kernelReal;
		}
	}

	fftwf_execute(convolutionPlan);

	// find best score
	for (int32_t y = 0; y < convolutionHeight; ++y) {
		for (int32_t x = 0; x < convolutionWidth; ++x) {
			auto index = y * maxWidth + x;
			auto real = input[index][0];
			auto imag = input[index][1];

			auto score = std::sqrt(real * real + imag * imag);

			if (score > ret.score) {
				ret.score = score;
				ret.x = x;
				ret.y = y;
			}
		}
	}

	// destroy fftw objects
	lock.lock();
	fftwf_destroy_plan(inputPlan);
	fftwf_destroy_plan(kernelPlan);
	fftwf_destroy_plan(convolutionPlan);

	fftwf_free(input);
	fftwf_free(kernelInput);
	fftwf_free(output);
	fftwf_free(kernelOutput);
	lock.unlock();

	return ret;
}

std::vector<ConvolutionScore> Generator::Impl::getScoresForGlyph(
	GlyphVector2D glyphVector, GeneratorConfig const& config
) {
	std::vector<ConvolutionScore> ret;

	// create kernel ids
	std::vector<int32_t> kernelIds(config.kernels.size());
	std::iota(kernelIds.begin(), kernelIds.end(), 0);

	// repeat for every object added to glyph
	for (int32_t objectIndex = 0; objectIndex < config.objectsPerGlyph; ++objectIndex) {
		std::mutex mutex;
		ConvolutionScore bestScore;

		struct Body {
			using argument_type = int32_t;
			Generator::Impl const* impl;
			GlyphVector2D const& glyphVector;
			GeneratorConfig const& config;
			std::mutex& mutex;
			ConvolutionScore& bestScore;

			Body(
				Generator::Impl const* impl,
				GlyphVector2D const& glyphVector,
				GeneratorConfig const& config,
				std::mutex& mutex,
				ConvolutionScore& bestScore
			) :
				impl(impl),
				glyphVector(glyphVector),
				config(config),
				mutex(mutex),
				bestScore(bestScore)
			{}

			void operator()(int32_t id, oneapi::tbb::feeder<int32_t>& feeder) const {
				// calculate convolution score
				auto score = impl->getConvolutionScore(glyphVector, config.kernels[id], config, mutex);
				score.kernelId = id;

				// if better than best, update best
				std::lock_guard<std::mutex> lock(mutex);
				if (score.score > bestScore.score) {
					bestScore = score;
				}
			}
		};

		// for each convolution id
		oneapi::tbb::parallel_for_each(kernelIds.begin(), kernelIds.end(), Body(
			this,
			glyphVector,
			config,
			mutex,
			bestScore
		));

		if (bestScore.score < config.minScore) {
			break;
		}

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

		// add the score to the list
		ret.push_back(bestScore);
	}

	return ret;
}

std::vector<CreatedObject> Generator::Impl::create(
	std::u32string const& text, GeneratorConfig const& config
) {
	// get all unique glyphs in text
	sf::Font font;
	font.loadFromFile(config.fontPath);
	auto glyphs = this->getUniqueGlyphs(text, config, font);

	// get matrix representations for each
	sf::Image fontImage = font.getTexture(config.fontSize).copyToImage();
	auto glyphVectors = this->getGlyphVectors(glyphs, config, fontImage);

	// add the negative scores to blank spaces
	this->addNegativeScores(glyphVectors, config);

	std::map<char32_t, std::vector<ConvolutionScore>> scoreMap;

	// for each glyph
	for (auto& [codepoint, glyphVector] : glyphVectors) {
		// get the scores for the glyph
		auto scores = this->getScoresForGlyph(glyphVector, config);

		// add the scores to the map
		scoreMap[codepoint] = scores;
	}

	// create the text 
	sf::Text textObject;
	textObject.setFont(font);
	textObject.setCharacterSize(config.fontSize);
	textObject.setFillColor(sf::Color::White);
	textObject.setString(sf::String::fromUtf32(text.begin(), text.end()));
	

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
			object.x = cursor.x + score.x;
			object.y = cursor.y + score.y;
			object.objectId = kernel.objectId;
			object.scale = kernel.scale;
			object.rotation = kernel.rotation;

			// add the object to the list
			ret.push_back(object);
		}
	}

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