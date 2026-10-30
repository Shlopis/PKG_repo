#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <QString>
#include <QVector>
#include "virtualgrid.h"

struct BenchmarkResult {
    QString name;
    QString primitive;
    double timeMs;
};

namespace Benchmark {
BenchmarkResult runLine(int algoIndex, int numLines, const VirtualGrid& sample);
BenchmarkResult runCircle(int algoIndex, int numCircles, const VirtualGrid& sample);
}

#endif // BENCHMARK_H
