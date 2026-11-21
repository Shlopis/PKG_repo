#include "weileratherton.h"
#include <algorithm>
#include <cmath>

namespace WeilerAtherton {

double polygonArea(const QVector<Point>& points) {
    double sum = 0;
    for (int i = 0, count = points.size(); i < count; ++i) {
        const Point& current = points[i];
        const Point& next = points[(i + 1) % count];
        sum += current.x * next.y - next.x * current.y;
    }
    return sum * 0.5;
}

// Лежит ли точка на отрезке ab (с точностью e):
static bool pointOnSegment(const Point& point, const Point& a, const Point& b) {
    const double cross = (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);
    if (std::fabs(cross) > Eps::E * std::max(1.0, std::fabs(b.x - a.x) + std::fabs(b.y - a.y)))
        return false;
    return Eps::ge(point.x, std::min(a.x, b.x)) && Eps::le(point.x, std::max(a.x, b.x)) && Eps::ge(point.y, std::min(a.y, b.y)) && Eps::le(point.y, std::max(a.y, b.y));
}

// Принадлежность точки полигону. Граница учитывается
bool pointInPolygon(const QVector<Point>& points, const Point& point) {
    const int count = points.size();
    if (count < 3) return false;
    for (int i = 0, j = count - 1; i < count; j = i++)
        if (pointOnSegment(point, points[i], points[j]))
            return true;
    bool inside = false; // четность
    for (int i = 0, j = count - 1; i < count; j = i++) {
        const Point& a = points[i];
        const Point& b = points[j];
        const bool aAbove = Eps::gt(a.y, point.y);
        const bool bAbove = Eps::gt(b.y, point.y);
        if (aAbove != bAbove) {
            const double intersectX = a.x + (point.y - a.y) * (b.x - a.x) / (b.y - a.y);
            if (Eps::lt(point.x, intersectX))
                inside = !inside;
        }
    }
    return inside;
}

//строгое пересечение
static bool segmentsIntersectProper(const Point& a, const Point& b, const Point& c, const Point& d) {
    const double d1 = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    const double d2 = (b.x - a.x) * (d.y - a.y) - (b.y - a.y) * (d.x - a.x);
    const double d3 = (d.x - c.x) * (a.y - c.y) - (d.y - c.y) * (a.x - c.x);
    const double d4 = (d.x - c.x) * (b.y - c.y) - (d.y - c.y) * (b.x - c.x);
    return Eps::gt(d1, 0) != Eps::gt(d2, 0) && Eps::gt(d3, 0) != Eps::gt(d4, 0);
}

// поиск самопереес. несм. ребер
bool hasSelfIntersections(const QVector<Point>& points) {
    const int count = points.size();
    for (int i = 0; i < count; ++i)
        for (int j = i + 2; j < count; ++j) {
            if (i == 0 && j == count - 1) continue; // 1-я и последняя смежны
            if (segmentsIntersectProper(points[i], points[(i + 1) % count], points[j], points[(j + 1) % count]))
                return true;
        }
    return false;
}

// Точка пересечения ребра полигона с ребром отсекателя.
struct Intersection {
    Point point;
    double subjectT = 0;  // параметр вдоль ребра полигона [0,1] для сортировки на ребре
    double clipT = 0;  // параметр вдоль ребра отсекателя [0,1] для сортировки на ребре
    int subjectEdge = -1; // номер ребра полигона, на котором пересечение
    int clipEdge = -1; // номер ребра отсекателя, на котором пересечение
    bool entering = false; // ребро идет снаружи внутрь - true
};

//узел в списках
struct ListNode {
    Point point;
    bool isIntersection = false;
    bool entering = false; // то же что и в первой структуре
    bool visited = false;
    int partner = -1; // индекс пары
};

//алгоритм
QVector<PolygonContour> clip(const QVector<PolygonContour>& subject, const QVector<Point>& clipPolygon) {
    QVector<PolygonContour> result;
    const int clipCount = clipPolygon.size();
    if (clipCount < 3)
        return result;
    if (Eps::eq(polygonArea(clipPolygon), 0.0)) return result; // площадь = 0

    for (const PolygonContour& contour : subject) {
        //вырожденные случчаи, нормализация обхода
        QVector<Point> subjectPoints;
        for (const Point& point : contour.pts) {//не даем подряд идущие повторы вершин
            if (subjectPoints.isEmpty() ||
                std::fabs(point.x - subjectPoints.back().x) > Eps::E ||
                std::fabs(point.y - subjectPoints.back().y) > Eps::E)
                subjectPoints.push_back(point);
        }
        while (subjectPoints.size() > 1 && std::fabs(subjectPoints.first().x - subjectPoints.back().x) <= Eps::E &&
               std::fabs(subjectPoints.first().y - subjectPoints.back().y) <= Eps::E)
            subjectPoints.pop_back(); // удаление повтора первой-последней в-ны

        const int subjectCount = subjectPoints.size(); //вырожд. сл.
        if (subjectCount < 3) continue;
        const double area = polygonArea(subjectPoints);
        if (Eps::eq(area, 0.0)) continue;
        if (area < 0)
            std::reverse(subjectPoints.begin(), subjectPoints.end()); // обход против час. стр.

        //1: пересечения рёбер полигона с рёбрами отсекателя
        QVector<Intersection> intersections;
        for (int i = 0; i < subjectCount; ++i) {
            const Point subjectStart = subjectPoints[i];
            const Point subjectEnd = subjectPoints[(i + 1) % subjectCount];
            const Point subjectDir{ subjectEnd.x - subjectStart.x, subjectEnd.y - subjectStart.y };
            for (int k = 0; k < clipCount; ++k) {
                const Point clipStart = clipPolygon[k];
                const Point clipEnd = clipPolygon[(k + 1) % clipCount];
                const Point clipDir{ clipEnd.x - clipStart.x, clipEnd.y - clipStart.y };
                const double denom = subjectDir.x * clipDir.y - subjectDir.y * clipDir.x;
                if (std::fabs(denom) <= Eps::E) continue;

                const Point delta{ clipStart.x - subjectStart.x, clipStart.y - subjectStart.y };
                double subjectT = (delta.x * clipDir.y - delta.y * clipDir.x) / denom;
                double clipT = (delta.x * subjectDir.y - delta.y * subjectDir.x) / denom;
                if ((subjectT < -Eps::E) || (subjectT > 1 + Eps::E) || (clipT < -Eps::E) || clipT > 1 + Eps::E)
                    continue; //пересечение вне отрезков
                subjectT = std::clamp(subjectT, 0.0, 1.0);
                clipT = std::clamp(clipT, 0.0, 1.0);

                const double crossSign = clipDir.x * subjectDir.y - clipDir.y * subjectDir.x;
                Intersection intersection;
                intersection.point = Point{
                    subjectStart.x + subjectDir.x * subjectT,
                    subjectStart.y + subjectDir.y * subjectT
                };
                intersection.subjectT = subjectT;
                intersection.clipT = clipT;
                intersection.subjectEdge = i;
                intersection.clipEdge = k;
                intersection.entering = crossSign > 0;
                intersections.push_back(intersection);
            }
        }

        // Дедупликация совпавших точек
        for (int a = 0; a < intersections.size(); ++a)
            for (int b = intersections.size() - 1; b > a; --b)
                if (intersections[b].subjectEdge == intersections[a].subjectEdge &&
                    std::fabs(intersections[b].point.x - intersections[a].point.x) <= Eps::E &&
                    std::fabs(intersections[b].point.y - intersections[a].point.y) <= Eps::E)
                    intersections.remove(b);

        //2: если нет пересечений
        if (intersections.isEmpty()) {
            if (pointInPolygon(clipPolygon, subjectPoints[0])) // полигон внутри окна
                result.push_back(PolygonContour{ subjectPoints, contour.hole });
            else if (pointInPolygon(subjectPoints, clipPolygon[0])) // окно внутри полигона
                result.push_back(PolygonContour{ clipPolygon, contour.hole });
            continue;
        }

        //3: два связанных списка
        QVector<QVector<int>> bySubjectEdge(subjectCount), byClipEdge(clipCount); // распределение по ребрам
        for (int idx = 0; idx < intersections.size(); ++idx) {
            bySubjectEdge[intersections[idx].subjectEdge].push_back(idx);
            byClipEdge[intersections[idx].clipEdge].push_back(idx);
        }
        for (auto& list : bySubjectEdge) //сортировки по T
            std::sort(list.begin(), list.end(),
                      [&](int a, int b) { return intersections[a].subjectT < intersections[b].subjectT; });
        for (auto& list : byClipEdge)
            std::sort(list.begin(), list.end(),
                      [&](int a, int b) { return intersections[a].clipT < intersections[b].clipT; });

        QVector<ListNode> subjectList, clipList;
        QVector<int> subjectNodeOf(intersections.size()), clipNodeOf(intersections.size());
        for (int i = 0; i < subjectCount; ++i) {
            subjectList.push_back(ListNode{ subjectPoints[i], false, false, false, -1 });
            for (int idx : bySubjectEdge[i]) {
                subjectNodeOf[idx] = subjectList.size();
                subjectList.push_back(ListNode{
                    intersections[idx].point, true, intersections[idx].entering, false, -1
                });
            }
        }
        for (int k = 0; k < clipCount; ++k) {
            clipList.push_back(ListNode{ clipPolygon[k], false, false, false, -1 });
            for (int idx : byClipEdge[k]) {
                clipNodeOf[idx] = clipList.size();
                clipList.push_back(ListNode{
                    intersections[idx].point, true, intersections[idx].entering, false, -1
                });
            }
        }

        for (int idx = 0; idx < intersections.size(); ++idx) { // составление пар
            subjectList[subjectNodeOf[idx]].partner = clipNodeOf[idx];
            clipList[clipNodeOf[idx]].partner = subjectNodeOf[idx];
        }

        //4: обход списков
        for (int start = 0; start < subjectList.size(); ++start) {
            if (!subjectList[start].isIntersection || !subjectList[start].entering ||
                subjectList[start].visited)
                continue;

            QVector<Point> output;
            int subjectIndex = start;// текущая позиция (полигон)
            int clipIndex = -1; // текущая позиция (окно)
            bool onSubject = true; // текущий список
            bool closed = false; // замкнулись ли
            const int maxSteps = 2 * (subjectList.size() + clipList.size()) + 16;
            int counter = 0;

            while (counter++ < maxSteps) {
                if (onSubject) {
                    ListNode& node = subjectList[subjectIndex];
                    if (node.isIntersection && !node.entering) {
                        node.visited = true;
                        output.push_back(node.point);
                        clipIndex = (node.partner + 1) % clipList.size();
                        onSubject = false;
                    }
                    else {
                        if (node.isIntersection) {// вершина субъекта либо входа: остаёмся
                            if (node.visited && subjectIndex == start) {
                                closed = true;
                                break;
                            }
                            node.visited = true;
                        }
                        output.push_back(node.point);
                        subjectIndex = (subjectIndex + 1) % subjectList.size();
                    }
                }
                else {
                    ListNode& node = clipList[clipIndex];
                    if (node.isIntersection && node.entering) {
                        subjectIndex = node.partner;
                        onSubject = true;
                    }
                    else {
                        output.push_back(node.point);// вершина окна: остаёмся
                        clipIndex = (clipIndex + 1) % clipList.size();
                    }
                }
            }
            if (closed && output.size() >= 3 && !Eps::eq(polygonArea(output), 0.0))
                result.push_back(PolygonContour{ output, contour.hole });
        }
    }
    return result;
}

QVector<PolygonContour> clipByRect(const QVector<PolygonContour>& subject, const Rect& window) {
    if (window.degenerate()) return {};
    const QVector<Point> rect{
        Point{ window.xmin, window.ymin },
        Point{ window.xmax, window.ymin },
        Point{ window.xmax, window.ymax },
        Point{ window.xmin, window.ymax }
    };
    return clip(subject, rect);
}

} // namespace WeilerAtherton