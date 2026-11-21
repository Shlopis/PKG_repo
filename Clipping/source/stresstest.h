#ifndef STRESSTEST_H
#define STRESSTEST_H

#include "geom.h"

constexpr int STRESS_SEGMENTS = 600000;

struct StressReport {
    int total = 0;
    int nodes = 0;
    int candidates = 0;
    int visibleQuad = 0;
    int visibleBrute = 0;
    double buildMs = 0;
    double qtClipMs = 0;
    double bruteMs = 0;
};

StressReport runStress(const Rect& window);

#endif // STRESSTEST_H
