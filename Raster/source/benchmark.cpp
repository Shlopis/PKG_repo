#include "benchmark.h"
#include "rasterengine.h"
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <cmath>

namespace Benchmark {

static const double PI = 3.14159265;

static QString nameFor(int idx) {
    switch (idx) {
    case 0: return "Пошаговый";
    case 1: return "ЦДА";
    case 2: return "Брезенхем";
    case 3: return "Брезенхем (окружность)";
    case 4: return "Кастла–Питвея";
    case 5: return "Ву (линия)";
    case 6: return "Ву (окружность)";
    }
    return "?";
}

BenchmarkResult runLine(int algoIndex, int numLines, const VirtualGrid& sample) {
    struct Seg { int x0, y0, x1, y1; };
    QVector<Seg> segs;
    segs.reserve(numLines);

    auto* rng = QRandomGenerator::global();
    const double cx = sample.minX() + sample.width()  / 2.0;
    const double cy = sample.minY() + sample.height() / 2.0;
    const double maxLen = std::min(sample.width(), sample.height()) / 2.0;

    for (int i = 0; i < numLines; ++i) {
        int octant = i % 8;
        double angle = (octant + rng->generateDouble()) * (PI / 4.0);
        double len   = 1.0 + rng->generateDouble() * (maxLen - 1.0);
        Seg s;
        s.x0 = int(cx);
        s.y0 = int(cy);
        s.x1 = int(cx + len * std::cos(angle));
        s.y1 = int(cy + len * std::sin(angle));
        segs.append(s);
    }

    VirtualGrid grid(sample.minX(), sample.minY(), sample.width(), sample.height());
    QElapsedTimer timer;
    timer.start();
    for (const Seg& s : segs) {
        switch (algoIndex) {
        case 0: RasterEngine::lineStep(s.x0, s.y0, s.x1, s.y1, grid); break;
        case 1: RasterEngine::lineDDA(s.x0, s.y0, s.x1, s.y1, grid); break;
        case 2: RasterEngine::lineBresenham(s.x0, s.y0, s.x1, s.y1, grid); break;
        case 4: RasterEngine::lineCastlePitwey(s.x0, s.y0, s.x1, s.y1, grid); break;
        case 5: RasterEngine::lineWu(s.x0, s.y0, s.x1, s.y1, grid); break;
        default: return { nameFor(algoIndex), "линия", 0.0 };
        }
    }
    return { nameFor(algoIndex), "линия", timer.nsecsElapsed() / 1e6 };
}

BenchmarkResult runCircle(int algoIndex, int numCircles, const VirtualGrid& sample) {
    QVector<int> radii;
    radii.reserve(numCircles);

    auto* rng = QRandomGenerator::global();
    const int maxR = std::min(sample.width(), sample.height()) / 2;

    for (int i = 0; i < numCircles; ++i)
        radii.append(1 + rng->bounded(maxR));

    VirtualGrid grid(sample.minX(), sample.minY(), sample.width(), sample.height());
    QElapsedTimer timer;
    timer.start();
    for (int r : radii) {
        switch (algoIndex) {
        case 3: RasterEngine::circleBresenham(r, grid); break;
        case 6: RasterEngine::circleWu(r, grid); break;
        default: return { nameFor(algoIndex), "окружность", 0.0 };
        }
    }
    return { nameFor(algoIndex), "окружность", timer.nsecsElapsed() / 1e6 };
}

} // namespace Benchmark