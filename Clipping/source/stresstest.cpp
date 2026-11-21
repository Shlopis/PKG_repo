#include "stresstest.h"
#include "cohensutherland.h"
#include "quadtree.h"
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <cmath>

StressReport runStress(const Rect& window) {
    StressReport report;
    report.total = STRESS_SEGMENTS;

    auto* rng = QRandomGenerator::global();
    const double PI = 3.14159265;

    QVector<Segment> segments(STRESS_SEGMENTS);
    for (Segment& segment : segments) {
        const double centerX = -90.0 + 180.0 * rng->generateDouble();
        const double centerY = -90.0 + 180.0 * rng->generateDouble();
        const double angle = rng->generateDouble() * 2.0 * PI;
        const double length = 1.0 + rng->generateDouble() * 19.0;
        segment.p1 = Point{ centerX, centerY };
        segment.p2 = Point{ centerX + length * std::cos(angle), centerY + length * std::sin(angle)
        };
    }

    const Rect domain{ -110, -110, 110, 110 };
    QuadTree quadTree;
    QElapsedTimer timer;

    timer.start();
    quadTree.build(segments, domain, 10, 16);
    report.buildMs = timer.nsecsElapsed() / 1e6;
    report.nodes = quadTree.nodeCount();

    timer.restart();
    const QVector<int> candidates = quadTree.query(window);
    report.candidates = candidates.size();
    for (int id : candidates)
        if (clipSegmentCS(segments[id], window).visible)
            ++report.visibleQuad;
    report.qtClipMs = timer.nsecsElapsed() / 1e6;

    timer.restart();
    for (int id = 0; id < STRESS_SEGMENTS; ++id)
        if (clipSegmentCS(segments[id], window).visible)
            ++report.visibleBrute;
    report.bruteMs = timer.nsecsElapsed() / 1e6;

    return report;
}