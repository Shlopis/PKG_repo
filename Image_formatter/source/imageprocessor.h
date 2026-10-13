#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include "imagewrapper.h"
#include <array>

struct HistogramData {
    std::array<int, 256> red{};
    std::array<int, 256> green{};
    std::array<int, 256> blue{};
};

namespace ImageProcessor {

HistogramData calculateHistogram(const ImageWrapper& wrapper, int numThreads = 4);

void applyLinearContrast(ImageWrapper& wrapper, int numThreads = 4);  // 0 = alfa · I + beta (a = 1.5; b = 0)

void applyHistogramEqualization(ImageWrapper& wrapper, int numThreads = 4);

void applyHistogramEqualizationRGB(ImageWrapper& wrapper, int numThreads = 4);

void applyHistogramEqualizationHSV(ImageWrapper& wrapper, int numThreads = 4);

void applyInversion(ImageWrapper& wrapper, int numThreads = 4);

void applyThreshold(ImageWrapper& wrapper, uchar threshold = 128, int numThreads = 4);

void applyGammaCorrection(ImageWrapper& wrapper, double gamma = 0.5, int numThreads = 4);

void applyCLAHE(ImageWrapper& wrapper, int tilesX = 8, int tilesY = 8, double clipLimit = 2.0, int numThreads = 4);

void applyCLAHEColor(ImageWrapper& wrapper, int tilesX = 8, int tilesY = 8,  double clipLimit = 2.0, int numThreads = 4);

}

#endif // IMAGEPROCESSOR_H
