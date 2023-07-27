#include <MatrixOperations.hpp>
#include <fftw3.h>
#include <algorithm>

using namespace tulip::text;

template <>
Matrix<double>::Matrix(size_t width, size_t height) : width(width), height(height) {
    data = fftw_alloc_real(width * height);
}

template <>
Matrix<double>::~Matrix() {
    fftw_free(data);
}

template <>
void Matrix<double>::zero() {
    std::fill(data, data + width * height, 0);
}

template <>
void Matrix<double>::fill(double value) {
    std::fill(data, data + width * height, value);
}

template <>
Matrix<fftw_complex>::Matrix(size_t width, size_t height) : width(width), height(height) {
    data = fftw_alloc_complex(width * height);
}

template <>
Matrix<fftw_complex>::~Matrix() {
    fftw_free(data);
}

template <>
void Matrix<fftw_complex>::zero() {
    std::fill(*data, *data + width * height * 2, 0);
}

Plan::Plan(fftw_plan plan) : plan(plan) {}

Plan::~Plan() {
    fftw_destroy_plan(plan);
}

Convolution::Convolution(Matrix<double>& input, Matrix<double>& kernel, Matrix<double>& output) :
    input(input),
    kernel(kernel),
    output(output),
    inputResult(input.width, input.height),
    kernelResult(kernel.width, kernel.height),
    kernelPlan(fftw_plan_dft_r2c_2d(kernel.width, kernel.height, kernel.data, kernelResult.data, FFTW_ESTIMATE)),
    inputPlan(fftw_plan_dft_r2c_2d(input.width, input.height, input.data, inputResult.data, FFTW_ESTIMATE)),
    outputPlan(fftw_plan_dft_c2r_2d(output.width, output.height, inputResult.data, output.data, FFTW_ESTIMATE)) {

    }

Convolution::~Convolution() {
    fftw_free(inputResult.data);
    fftw_free(kernelResult.data);
    fftw_destroy_plan(kernelPlan.plan);
    fftw_destroy_plan(inputPlan.plan);
    fftw_destroy_plan(outputPlan.plan);
}

void Convolution::execute() {
    fftw_execute(kernelPlan.plan);
    fftw_execute(inputPlan.plan);

    for (size_t i = 0; i < inputResult.width * inputResult.height; ++i) {
        auto const inputReal = inputResult.data[i][0];
        auto const inputImag = inputResult.data[i][1];
        auto const kernelReal = kernelResult.data[i][0];
        auto const kernelImag = kernelResult.data[i][1];

        inputResult.data[i][0] = inputReal * kernelReal - inputImag * kernelImag;
        inputResult.data[i][1] = inputReal * kernelImag + inputImag * kernelReal;
    }

    fftw_execute(outputPlan.plan);

    for (size_t i = 0; i < inputResult.width * inputResult.height; ++i) {
        output.data[i] /= inputResult.width * inputResult.height;
    }
}