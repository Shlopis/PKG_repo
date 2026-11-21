#ifndef GEOM_H
#define GEOM_H

#include <QVector>
#include <cmath>
#include <algorithm>

//ЭПСИЛОН-СРАВНЕНИЯ
namespace Eps {
constexpr double E = 1e-7;
inline bool eq(double a, double b) { return fabs(a - b) <= E; }
inline bool lt(double a, double b) { return a < b - E; }
inline bool gt(double a, double b) { return a > b + E; }
inline bool le(double a, double b) { return a <= b + E; }
inline bool ge(double a, double b) { return a >= b - E; }
}

struct Point { double x = 0, y = 0; };
struct Segment { Point p1, p2; };

struct Rect {
    double xmin = 0, ymin = 0, xmax = 0, ymax = 0;
    bool degenerate() const { return Eps::le(xmax, xmin) || Eps::le(ymax, ymin); }
};

struct PolygonContour{
    QVector<Point> pts;
    bool hole = false;
};

inline Rect segAABB(const Segment& segment) {
    return {
        std::min(segment.p1.x, segment.p2.x), std::min(segment.p1.y, segment.p2.y),
        std::max(segment.p1.x, segment.p2.x), std::max(segment.p1.y, segment.p2.y)
    };
}

inline bool rectIntersect(const Rect& a, const Rect& b) {
    return Eps::le(a.xmin, b.xmax) && Eps::le(b.xmin, a.xmax) && Eps::le(a.ymin, b.ymax) && Eps::le(b.ymin, a.ymax);
}

#endif // GEOM_H