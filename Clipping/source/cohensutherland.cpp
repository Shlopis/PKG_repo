#include "cohensutherland.h"

unsigned outcode(const Point& point, const Rect& window) {
    unsigned code = 0;
    if (Eps::gt(point.y, window.ymax))
        code |= OC_TOP;
    else if (Eps::lt(point.y, window.ymin))
        code |= OC_BOTTOM;
    if (Eps::gt(point.x, window.xmax))
        code |= OC_RIGHT;
    else if (Eps::lt(point.x, window.xmin))
        code |= OC_LEFT;
    return code;
}

ClipResult clipSegmentCS(const Segment& segment, const Rect& window) {
    ClipResult result;
    if (window.degenerate())
        return result;

    Point startPoint = segment.p1;
    Point endPoint = segment.p2;
    unsigned startCode = outcode(startPoint, window);
    unsigned endCode = outcode(endPoint, window);

    // Тривиальные случаи
    if ((startCode | endCode) == 0) { //полностью в окне
        result.visible = true;
        result.segment = segment;
        return result;
    }
    if ((startCode & endCode) != 0) // полностью вне окна
        return result;

    // Основной цикл: заменяется один внешний конец на пересечение с границей.
    for (int i = 0; i < 8; ++i) {
        const unsigned currentCode = startCode ? startCode : endCode;
        if (currentCode == 0) break;

        Point intersectionPoint;
        if (currentCode & OC_TOP) { //0001
            const double dy = endPoint.y - startPoint.y;
            if (dy == 0.0)
                return result;
            const double t = (window.ymax - startPoint.y) / dy;
            intersectionPoint = { startPoint.x + t * (endPoint.x - startPoint.x), window.ymax };
        }
        else if (currentCode & OC_BOTTOM) { //0010
            const double dy = endPoint.y - startPoint.y;
            if (dy == 0.0) return result;
            const double t = (window.ymin - startPoint.y) / dy;
            intersectionPoint = { startPoint.x + t * (endPoint.x - startPoint.x), window.ymin };
        }
        else if (currentCode & OC_RIGHT) { //0100
            const double dx = endPoint.x - startPoint.x;
            if (dx == 0.0) return result;
            const double t = (window.xmax - startPoint.x) / dx;
            intersectionPoint = { window.xmax, startPoint.y + t * (endPoint.y - startPoint.y) };
        }
        else {  //1000
            const double dx = endPoint.x - startPoint.x;
            if (dx == 0.0) return result;
            const double t = (window.xmin - startPoint.x) / dx;
            intersectionPoint = { window.xmin, startPoint.y + t * (endPoint.y - startPoint.y) };
        }

        if (startCode){
            startPoint = intersectionPoint;
            startCode = outcode(startPoint, window);
        }
        else {
            endPoint = intersectionPoint;
            endCode = outcode(endPoint, window);
        }

        if ((startCode & endCode) != 0) //вне окна
            return result;
    }

    if ((startCode | endCode) == 0) {
        result.visible = true;
        result.segment = { startPoint, endPoint };
    }
    return result;
}