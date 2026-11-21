#ifndef WEILERATHERTON_H
#define WEILERATHERTON_H

#include "geom.h"
//обход против часовой стрелки.
namespace WeilerAtherton {

QVector<PolygonContour> clip(const QVector<PolygonContour>& subject, const QVector<Point>& clipPolygon);
QVector<PolygonContour> clipByRect(const QVector<PolygonContour>& subject, const Rect& window);

double polygonArea(const QVector<Point>& points); // знакоплощадь
bool pointInPolygon(const QVector<Point>& points, const Point& point); // критерий чёт-нечет
bool hasSelfIntersections(const QVector<Point>& points); // перебор несмежных рёбер
}

#endif // WEILERATHERTON_H