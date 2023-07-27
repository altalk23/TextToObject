#pragma once

#include <fftw3.h>

namespace tulip::text {

    template <class Type>
	struct Matrix {
        size_t width;
        size_t height;
        Type* data;

        Type& operator()(size_t x, size_t y) {
            return data[y * width + x];
        }

        void fill(Type value);
        void zero();

        Matrix(size_t width, size_t height);
        ~Matrix();
    };

    struct Plan {
        fftw_plan plan;

        Plan(fftw_plan plan);
        ~Plan();
    };

    struct Convolution {
        Matrix<double>& input;
        Matrix<double>& kernel;
        Matrix<double>& output;
        Matrix<fftw_complex> inputResult;
        Matrix<fftw_complex> kernelResult;
        Plan kernelPlan;
        Plan inputPlan;
        Plan outputPlan;

        Convolution(Matrix<double>& input, Matrix<double>& kernel, Matrix<double>& output);
        ~Convolution();

        void execute();
    };
}