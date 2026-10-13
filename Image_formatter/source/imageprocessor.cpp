#include "imageprocessor.h"
#include <QtConcurrent>
#include <QFuture>
#include <QDebug>
#include <algorithm>
#include <cmath>

namespace ImageProcessor {

struct LocalHistograms {
    std::array<int, 256> r{};
    std::array<int, 256> g{};
    std::array<int, 256> b{};
};

struct HSV { double h, s, v; };

struct TileData { std::array<uchar, 256> cdf{}; };

static HSV rgbToHsv(int r, int g, int b) {
    r = std::clamp(r, 0, 255);
    g = std::clamp(g, 0, 255);
    b = std::clamp(b, 0, 255);

    double rd = r / 255.0, gd = g / 255.0, bd = b / 255.0;
    double cmax = std::max({rd, gd, bd});
    double cmin = std::min({rd, gd, bd});
    double delta = cmax - cmin;

    HSV hsv;
    hsv.v = cmax;
    hsv.s = (cmax == 0) ? 0 : delta / cmax;

    if (delta == 0)
        hsv.h = 0;
    else if (cmax == rd)
        hsv.h = 60.0 * std::fmod((gd - bd) / delta, 6.0);
    else if (cmax == gd)
        hsv.h = 60.0 * ((bd - rd) / delta + 2.0);
    else
        hsv.h = 60.0 * ((rd - gd) / delta + 4.0);

    if (hsv.h < 0) hsv.h += 360.0;
    if (hsv.h >= 360.0) hsv.h = std::fmod(hsv.h, 360.0);
    if (std::isnan(hsv.h) || std::isinf(hsv.h)) hsv.h = 0;
    if (std::isnan(hsv.s) || std::isinf(hsv.s)) hsv.s = 0;
    if (std::isnan(hsv.v) || std::isinf(hsv.v)) hsv.v = 0;
    return hsv;
}

static void hsvToRgb(const HSV& hsv, int& r, int& g, int& b) {
    double h = std::fmod(hsv.h, 360.0);
    if (h < 0) h += 360.0;
    double s = std::clamp(hsv.s, 0.0, 1.0);
    double v = std::clamp(hsv.v, 0.0, 1.0);

    double C = v * s;
    double X = C * (1.0 - std::abs(std::fmod(h / 60.0, 2.0) - 1.0));
    double m = v - C;

    double rd, gd, bd;
    if (h < 60)       { rd = C; gd = X; bd = 0; }
    else if (h < 120) { rd = X; gd = C; bd = 0; }
    else if (h < 180) { rd = 0; gd = C; bd = X; }
    else if (h < 240) { rd = 0; gd = X; bd = C; }
    else if (h < 300) { rd = X; gd = 0; bd = C; }
    else              { rd = C; gd = 0; bd = X; }

    r = std::clamp(static_cast<int>((rd + m) * 255), 0, 255);
    g = std::clamp(static_cast<int>((gd + m) * 255), 0, 255);
    b = std::clamp(static_cast<int>((bd + m) * 255), 0, 255);
}

static std::array<uchar, 256> buildLUT(const std::array<int, 256>& hist, int totalPixels) {
    std::array<int, 256> cdf{};
    cdf[0] = hist[0];
    for (int i = 1; i < 256; ++i)
        cdf[i] = cdf[i - 1] + hist[i];

    int cdfMin = 0;
    for (int i = 0; i < 256; ++i) {
        if (cdf[i] > 0) { cdfMin = cdf[i]; break; }
    }

    double denom = totalPixels - cdfMin;
    if (denom <= 0) denom = 1;

    std::array<uchar, 256> lut{};
    for (int i = 0; i < 256; ++i) {
        double val = (cdf[i] - cdfMin) / denom * 255.0;
        lut[i] = static_cast<uchar>(std::clamp(static_cast<int>(std::round(val)), 0, 255));
    }
    return lut;
}

static void applyLUTToGray(ImageWrapper& wrapper, const std::array<uchar, 256>& lut, int numThreads) {
    int height = wrapper.height();
    if (height < numThreads) numThreads = height;
    int rowsPerThread = height / numThreads;

    std::vector<QFuture<void>> futures;
    for (int i = 0; i < numThreads; ++i) {
        int startY = i * rowsPerThread;
        int endY = (i == numThreads - 1) ? height : (i + 1) * rowsPerThread;

        futures.push_back(QtConcurrent::run([&, startY, endY]() {
            for (int y = startY; y < endY; ++y) {
                uchar* line = wrapper.getGrayScanLineMutable(y);
                for (int x = 0; x < wrapper.width(); ++x)
                    line[x] = lut[line[x]];
            }
        }));
    }
    for (auto& f : futures) f.waitForFinished();
}

static void applyLUTToRGB(ImageWrapper& wrapper, const std::array<uchar, 256>& lutR, const std::array<uchar, 256>& lutG, const std::array<uchar, 256>& lutB,int numThreads)
{
    int height = wrapper.height();
    if (height < numThreads) numThreads = height;
    int rowsPerThread = height / numThreads;

    std::vector<QFuture<void>> futures;
    for (int i = 0; i < numThreads; ++i) {
        int startY = i * rowsPerThread;
        int endY = (i == numThreads - 1) ? height : (i + 1) * rowsPerThread;

        futures.push_back(QtConcurrent::run([&, startY, endY]() {
            for (int y = startY; y < endY; ++y) {
                QRgb* line = wrapper.getRgbScanLineMutable(y);
                for (int x = 0; x < wrapper.width(); ++x) {
                    QRgb px = line[x];
                    line[x] = qRgb(lutR[qRed(px)], lutG[qGreen(px)], lutB[qBlue(px)]);
                }
            }
        }));
    }
    for (auto& f : futures) f.waitForFinished();
}

static void applyLUTToRGB(ImageWrapper& wrapper, const std::array<uchar, 256>& lut, int numThreads){
    applyLUTToRGB(wrapper, lut, lut, lut, numThreads);
}

HistogramData calculateHistogram(const ImageWrapper& wrapper, int numThreads) {
    HistogramData result;
    int height = wrapper.height();
    if (height == 0) return result;
    if (height < numThreads) numThreads = height;
    int rowsPerThread = height / numThreads;

    auto processStrip = [&](int startY, int endY) -> LocalHistograms {
        LocalHistograms local;
        if (wrapper.isGrayscale()) {
            for (int y = startY; y < endY; ++y) {
                const uchar* line = wrapper.getGrayScanLineConst(y);
                for (int x = 0; x < wrapper.width(); ++x)
                    local.r[line[x]]++;
            }
        } else {
            for (int y = startY; y < endY; ++y) {
                const QRgb* line = wrapper.getRgbScanLineConst(y);
                for (int x = 0; x < wrapper.width(); ++x) {
                    QRgb px = line[x];
                    local.r[qRed(px)]++;
                    local.g[qGreen(px)]++;
                    local.b[qBlue(px)]++;
                }
            }
        }
        return local;
    };

    std::vector<QFuture<LocalHistograms>> futures;
    for (int i = 0; i < numThreads; ++i) {
        int startY = i * rowsPerThread;
        int endY = (i == numThreads - 1) ? height : (i + 1) * rowsPerThread;
        futures.push_back(QtConcurrent::run(processStrip, startY, endY));
    }

    for (auto& f : futures) {
        LocalHistograms local = f.result();
        for (int i = 0; i < 256; ++i) {
            result.red[i]   += local.r[i];
            result.green[i] += local.g[i];
            result.blue[i]  += local.b[i];
        }
    }
    return result;
}

void applyLinearContrast(ImageWrapper& wrapper, int numThreads) {
    wrapper.ensureDetached();

    double alpha = 1.5;
    double beta  = 0.0;

    std::array<uchar, 256> lut{};
    for (int i = 0; i < 256; ++i) {
        int val = static_cast<int>(alpha * i + beta);
        lut[i] = static_cast<uchar>(std::clamp(val, 0, 255));
    }

    if (wrapper.isGrayscale())
        applyLUTToGray(wrapper, lut, numThreads);
    else
        applyLUTToRGB(wrapper, lut, numThreads);
}

void applyHistogramEqualization(ImageWrapper& wrapper, int numThreads) {
    if (!wrapper.isGrayscale()) {
        qDebug() << "Выравнивание гистограммы: только для Grayscale";
        return;
    }
    wrapper.ensureDetached();

    HistogramData hist = calculateHistogram(wrapper, numThreads);
    int totalPixels = wrapper.width() * wrapper.height();
    auto lut = buildLUT(hist.red, totalPixels);
    applyLUTToGray(wrapper, lut, numThreads);
}

void applyHistogramEqualizationRGB(ImageWrapper& wrapper, int numThreads) {
    if (wrapper.isGrayscale()) return;
    wrapper.ensureDetached();

    HistogramData hist = calculateHistogram(wrapper, numThreads);
    int totalPixels = wrapper.width() * wrapper.height();

    auto lutR = buildLUT(hist.red, totalPixels);
    auto lutG = buildLUT(hist.green, totalPixels);
    auto lutB = buildLUT(hist.blue, totalPixels);

    applyLUTToRGB(wrapper, lutR, lutG, lutB, numThreads);
}

void applyHistogramEqualizationHSV(ImageWrapper& wrapper, int numThreads) {
    if (wrapper.isGrayscale()) return;
    wrapper.ensureDetached();

    int width  = wrapper.width();
    int height = wrapper.height();
    int totalPixels = width * height;

    std::vector<uchar> vChannel(totalPixels);
    for (int y = 0; y < height; ++y) {
        const QRgb* line = wrapper.getRgbScanLineConst(y);
        for (int x = 0; x < width; ++x) {
            QRgb px = line[x];
            HSV hsv = rgbToHsv(qRed(px), qGreen(px), qBlue(px));
            vChannel[y * width + x] = static_cast<uchar>(hsv.v * 255.0);
        }
    }

    std::array<int, 256> vHist{};
    for (int i = 0; i < totalPixels; ++i)
        vHist[vChannel[i]]++;

    auto lutV = buildLUT(vHist, totalPixels);

    if (height < numThreads) numThreads = height;
    int rowsPerThread = height / numThreads;

    std::vector<QFuture<void>> futures;
    for (int i = 0; i < numThreads; ++i) {
        int startY = i * rowsPerThread;
        int endY = (i == numThreads - 1) ? height : (i + 1) * rowsPerThread;

        futures.push_back(QtConcurrent::run([&, startY, endY]() {
            for (int y = startY; y < endY; ++y) {
                QRgb* line = wrapper.getRgbScanLineMutable(y);
                for (int x = 0; x < width; ++x) {
                    QRgb px = line[x];
                    HSV hsv = rgbToHsv(qRed(px), qGreen(px), qBlue(px));
                    hsv.v = lutV[vChannel[y * width + x]] / 255.0;
                    int r, g, b;
                    hsvToRgb(hsv, r, g, b);
                    line[x] = qRgb(r, g, b);
                }
            }
        }));
    }
    for (auto& f : futures) f.waitForFinished();
}

void applyInversion(ImageWrapper& wrapper, int numThreads) {
    wrapper.ensureDetached();

    std::array<uchar, 256> lut{};
    for (int i = 0; i < 256; ++i)
        lut[i] = static_cast<uchar>(255 - i);

    if (wrapper.isGrayscale())
        applyLUTToGray(wrapper, lut, numThreads);
    else
        applyLUTToRGB(wrapper, lut, numThreads);
}

void applyThreshold(ImageWrapper& wrapper, uchar threshold, int numThreads) {
    if (!wrapper.isGrayscale()) {
        qDebug() << "Порог: только для Grayscale";
        return;
    }
    wrapper.ensureDetached();

    std::array<uchar, 256> lut{};
    for (int i = 0; i < 256; ++i)
        lut[i] = (i > threshold) ? 255 : 0;

    applyLUTToGray(wrapper, lut, numThreads);
}

void applyGammaCorrection(ImageWrapper& wrapper, double gamma, int numThreads) {
    wrapper.ensureDetached();
    if (gamma <= 0) gamma = 0.1;

    std::array<uchar, 256> lut{};
    for (int i = 0; i < 256; ++i) {
        double corrected = std::pow(i / 255.0, gamma);
        int val = static_cast<int>(corrected * 255.0 + 0.5);
        lut[i] = static_cast<uchar>(std::clamp(val, 0, 255));
    }

    if (wrapper.isGrayscale())
        applyLUTToGray(wrapper, lut, numThreads);
    else
        applyLUTToRGB(wrapper, lut, numThreads);
}

static void clipAndRedistribute(std::array<int, 256>& hist, int clipLimit) {
    int excess = 0;
    for (int i = 0; i < 256; ++i) {
        if (hist[i] > clipLimit) {
            excess += hist[i] - clipLimit;
            hist[i] = clipLimit;
        }
    }

    int redistribute = excess / 256;
    int remainder = excess % 256;

    for (int i = 0; i < 256; ++i) {
        hist[i] += redistribute;
        if (i < remainder) {
            hist[i]++;
        }
    }
}

static std::array<uchar, 256> buildTileCDF(const std::array<int, 256>& hist, int tilePixels) {
    std::array<int, 256> cdf{};
    cdf[0] = hist[0];
    for (int i = 1; i < 256; ++i) {
        cdf[i] = cdf[i - 1] + hist[i];
    }

    int cdfMin = 0;
    for (int i = 0; i < 256; ++i) {
        if (cdf[i] > 0) { cdfMin = cdf[i]; break; }
    }

    double denom = tilePixels - cdfMin;
    if (denom <= 0) denom = 1;

    std::array<uchar, 256> lut{};
    for (int i = 0; i < 256; ++i) {
        double val = (cdf[i] - cdfMin) / denom * 255.0;
        lut[i] = static_cast<uchar>(std::clamp(static_cast<int>(std::round(val)), 0, 255));
    }
    return lut;
}

void applyCLAHE(ImageWrapper& wrapper, int tilesX, int tilesY, double clipLimit, int numThreads)
{
    if (!wrapper.isGrayscale()) {
        qDebug() << "CLAHE: только для Grayscale (для цветных используйте applyCLAHEColor)";
        return;
    }
    wrapper.ensureDetached();

    int width = wrapper.width();
    int height = wrapper.height();

    if (width < tilesX || height < tilesY) {
        qDebug() << "CLAHE: изображение слишком маленькое для сетки" << tilesX << "x" << tilesY;
        return;
    }

    int tileW = width / tilesX;
    int tileH = height / tilesY;
    int tilePixels = tileW * tileH;

    int actualClipLimit = static_cast<int>(clipLimit * tilePixels / 256.0);
    if (actualClipLimit < 1) actualClipLimit = 1;

    std::vector<TileData> tiles(tilesX * tilesY);

    for (int ty = 0; ty < tilesY; ++ty) {
        for (int tx = 0; tx < tilesX; ++tx) {
            std::array<int, 256> hist{};

            int startX = tx * tileW;
            int startY = ty * tileH;
            int endX = (tx == tilesX - 1) ? width : startX + tileW;
            int endY = (ty == tilesY - 1) ? height : startY + tileH;

            for (int y = startY; y < endY; ++y) {
                const uchar* line = wrapper.getGrayScanLineConst(y);
                for (int x = startX; x < endX; ++x) {
                    hist[line[x]]++;
                }
            }
            clipAndRedistribute(hist, actualClipLimit);

            int actualPixels = (endX - startX) * (endY - startY);
            tiles[ty * tilesX + tx].cdf = buildTileCDF(hist, actualPixels);
        }
    }

    if (height < numThreads) numThreads = height;
    int rowsPerThread = height / numThreads;

    std::vector<QFuture<void>> futures;
    for (int i = 0; i < numThreads; ++i) {
        int startY = i * rowsPerThread;
        int endY = (i == numThreads - 1) ? height : (i + 1) * rowsPerThread;

        futures.push_back(QtConcurrent::run([&, startY, endY]() {
            for (int y = startY; y < endY; ++y) {
                uchar* line = wrapper.getGrayScanLineMutable(y);
                for (int x = 0; x < width; ++x) {
                    uchar pixelVal = line[x];

                    double tileXf = (static_cast<double>(x) / tileW) - 0.5;
                    double tileYf = (static_cast<double>(y) / tileH) - 0.5;

                    int tx0 = static_cast<int>(std::floor(tileXf));
                    int ty0 = static_cast<int>(std::floor(tileYf));
                    int tx1 = tx0 + 1;
                    int ty1 = ty0 + 1;

                    tx0 = std::clamp(tx0, 0, tilesX - 1);
                    ty0 = std::clamp(ty0, 0, tilesY - 1);
                    tx1 = std::clamp(tx1, 0, tilesX - 1);
                    ty1 = std::clamp(ty1, 0, tilesY - 1);

                    double fx = tileXf - std::floor(tileXf);
                    double fy = tileYf - std::floor(tileYf);
                    fx = std::clamp(fx, 0.0, 1.0);
                    fy = std::clamp(fy, 0.0, 1.0);

                    double valTL = tiles[ty0 * tilesX + tx0].cdf[pixelVal];
                    double valTR = tiles[ty0 * tilesX + tx1].cdf[pixelVal];
                    double valBL = tiles[ty1 * tilesX + tx0].cdf[pixelVal];
                    double valBR = tiles[ty1 * tilesX + tx1].cdf[pixelVal];

                    double top = valTL * (1.0 - fx) + valTR * fx;
                    double bottom = valBL * (1.0 - fx) + valBR * fx;
                    double result = top * (1.0 - fy) + bottom * fy;

                    line[x] = static_cast<uchar>(std::clamp(static_cast<int>(std::round(result)), 0, 255));
                }
            }
        }));
    }
    for (auto& f : futures) f.waitForFinished();
}

void applyCLAHEColor(ImageWrapper& wrapper, int tilesX, int tilesY, double clipLimit, int numThreads) {
    if (wrapper.isGrayscale()) {
        applyCLAHE(wrapper, tilesX, tilesY, clipLimit, numThreads);
        return;
    }
    wrapper.ensureDetached();

    int width = wrapper.width();
    int height = wrapper.height();
    int totalPixels = width * height;

    if (width < tilesX || height < tilesY) return;

    int tileW = width / tilesX;
    int tileH = height / tilesY;
    int actualClipLimit = static_cast<int>(clipLimit * tileW * tileH / 256.0);
    if (actualClipLimit < 1) actualClipLimit = 1;

    std::vector<uchar> vChannel(totalPixels);
    for (int y = 0; y < height; ++y) {
        const QRgb* line = wrapper.getRgbScanLineConst(y);
        for (int x = 0; x < width; ++x) {
            QRgb px = line[x];
            HSV hsv = rgbToHsv(qRed(px), qGreen(px), qBlue(px));
            vChannel[y * width + x] = static_cast<uchar>(hsv.v * 255.0);
        }
    }
    std::vector<TileData> tiles(tilesX * tilesY);

    for (int ty = 0; ty < tilesY; ++ty) {
        for (int tx = 0; tx < tilesX; ++tx) {
            std::array<int, 256> hist{};

            int startX = tx * tileW;
            int startY = ty * tileH;
            int endX = (tx == tilesX - 1) ? width : startX + tileW;
            int endY = (ty == tilesY - 1) ? height : startY + tileH;

            for (int y = startY; y < endY; ++y) {
                for (int x = startX; x < endX; ++x) {
                    hist[vChannel[y * width + x]]++;
                }
            }

            clipAndRedistribute(hist, actualClipLimit);
            int actualPixels = (endX - startX) * (endY - startY);
            tiles[ty * tilesX + tx].cdf = buildTileCDF(hist, actualPixels);
        }
    }

    if (height < numThreads) numThreads = height;
    int rowsPerThread = height / numThreads;

    std::vector<QFuture<void>> futures;
    for (int i = 0; i < numThreads; ++i) {
        int startY = i * rowsPerThread;
        int endY = (i == numThreads - 1) ? height : (i + 1) * rowsPerThread;

        futures.push_back(QtConcurrent::run([&, startY, endY]() {
            for (int y = startY; y < endY; ++y) {
                QRgb* line = wrapper.getRgbScanLineMutable(y);
                for (int x = 0; x < width; ++x) {
                    int idx = y * width + x;
                    uchar vVal = vChannel[idx];

                    double tileXf = (static_cast<double>(x) / tileW) - 0.5;
                    double tileYf = (static_cast<double>(y) / tileH) - 0.5;

                    int tx0 = std::clamp(static_cast<int>(std::floor(tileXf)), 0, tilesX - 1);
                    int ty0 = std::clamp(static_cast<int>(std::floor(tileYf)), 0, tilesY - 1);
                    int tx1 = std::clamp(tx0 + 1, 0, tilesX - 1);
                    int ty1 = std::clamp(ty0 + 1, 0, tilesY - 1);

                    double fx = std::clamp(tileXf - std::floor(tileXf), 0.0, 1.0);
                    double fy = std::clamp(tileYf - std::floor(tileYf), 0.0, 1.0);

                    double valTL = tiles[ty0 * tilesX + tx0].cdf[vVal];
                    double valTR = tiles[ty0 * tilesX + tx1].cdf[vVal];
                    double valBL = tiles[ty1 * tilesX + tx0].cdf[vVal];
                    double valBR = tiles[ty1 * tilesX + tx1].cdf[vVal];

                    double top = valTL * (1.0 - fx) + valTR * fx;
                    double bottom = valBL * (1.0 - fx) + valBR * fx;
                    double newV = top * (1.0 - fy) + bottom * fy;

                    QRgb px = line[x];
                    HSV hsv = rgbToHsv(qRed(px), qGreen(px), qBlue(px));
                    hsv.v = std::clamp(newV / 255.0, 0.0, 1.0);

                    int r, g, b;
                    hsvToRgb(hsv, r, g, b);
                    line[x] = qRgb(r, g, b);
                }
            }
        }));
    }
    for (auto& f : futures) f.waitForFinished();
}
}
