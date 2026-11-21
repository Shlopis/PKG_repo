#include "scenepainter.h"
#include "weileratherton.h"
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QGuiApplication>
#include <cmath>
#include <algorithm>

static constexpr double MIN_CLIP_SIZE = 0.01;

static Rect polygonBounds(const QVector<Point>& polygon) {
    if (polygon.isEmpty()) return Rect{ 0, 0, 0, 0 };
    Rect b{ polygon[0].x, polygon[0].y, polygon[0].x, polygon[0].y };
    for (const Point& p : polygon) {
        b.xmin = std::min(b.xmin, p.x);
        b.ymin = std::min(b.ymin, p.y);
        b.xmax = std::max(b.xmax, p.x);
        b.ymax = std::max(b.ymax, p.y);
    }
    return b;
}

static bool sameRect(const Rect& a, const Rect& b) {
    return a.xmin == b.xmin && a.xmax == b.xmax && a.ymin == b.ymin && a.ymax == b.ymax;
}

ScenePainter::ScenePainter(QWidget* parent) : QWidget(parent) {
    setMinimumSize(500, 400);
    setMouseTracking(true);
}

void ScenePainter::setSegments(const QVector<Segment>& original, const QVector<Segment>& clipped) {
    originalSegments = original;
    clippedSegments = clipped;
    update();
}

void ScenePainter::setClipRect(const Rect& rect, bool has) {
    clipRect = rect;
    hasClipRect = has;
    update();
}

void ScenePainter::setClipPolygon(const QVector<Point>& polygon, bool has) {
    clipPolygon = polygon;
    hasClipPolygon = has;
    update();
}

void ScenePainter::setQuadTreeRects(const QVector<Rect>& rects) { quadTreeRects = rects; update(); }

void ScenePainter::setPolygons(const QVector<PolygonContour>& original, const QVector<PolygonContour>& clipped) {
    originalPolygons = original;
    clippedPolygons = clipped;
    update();
}

void ScenePainter::setFlags(bool showOriginal_, bool showClipped_, bool showQuadTree_, bool showPolygons_) {
    showOriginal = showOriginal_;
    showClipped = showClipped_;
    showQuadTree = showQuadTree_;
    showPolygons = showPolygons_;
    update();
}

void ScenePainter::clampView() {
    const double limit = WORLD_LIMIT;
    const double minScale = std::max(width() / (2.0 * limit), height() / (2.0 * limit));
    scale = std::clamp(scale, minScale, 1e5);
    const double halfWidth = width() / (2.0 * scale);
    const double halfHeight = height() / (2.0 * scale);
    centerX = std::clamp(centerX, -limit + halfWidth, limit - halfWidth);
    centerY = std::clamp(centerY, -limit + halfHeight, limit - halfHeight);
}

void ScenePainter::fitToContent() {
    const double limit = WORLD_LIMIT;
    Rect bounds{ -limit, -limit, limit, limit };
    bool hasBounds = false;

    auto extendBounds = [&](const Rect& rect) {
        Rect c{ std::max(rect.xmin, -limit), std::max(rect.ymin, -limit),
               std::min(rect.xmax,  limit), std::min(rect.ymax,  limit) };
        if (c.xmin > c.xmax || c.ymin > c.ymax) return;
        if (!hasBounds) { bounds = c; hasBounds = true; return; }
        bounds.xmin = std::min(bounds.xmin, c.xmin);
        bounds.ymin = std::min(bounds.ymin, c.ymin);
        bounds.xmax = std::max(bounds.xmax, c.xmax);
        bounds.ymax = std::max(bounds.ymax, c.ymax);
    };

    for (const Segment& s : originalSegments) extendBounds(segAABB(s));
    for (const Segment& s : clippedSegments)  extendBounds(segAABB(s));
    if (hasClipRect)    extendBounds(clipRect);
    if (hasClipPolygon) extendBounds(polygonBounds(clipPolygon));
    for (const PolygonContour& c : originalPolygons)
        for (const Point& p : c.pts) extendBounds({ p.x, p.y, p.x, p.y });

    const double w = std::max(1e-6, bounds.xmax - bounds.xmin);
    const double h = std::max(1e-6, bounds.ymax - bounds.ymin);
    scale = std::min(width() / w, height() / h) * 0.9;
    centerX = (bounds.xmin + bounds.xmax) * 0.5;
    centerY = (bounds.ymin + bounds.ymax) * 0.5;
    clampView();
    update();
}

void ScenePainter::wheelEvent(QWheelEvent* event) {
    const QPoint pos = event->position().toPoint();
    const double wxBefore = worldX(pos.x());
    const double wyBefore = worldY(pos.y());
    scale *= (event->angleDelta().y() > 0) ? 1.25 : 0.8;
    centerX = wxBefore - (pos.x() - width() * 0.5) / scale;
    centerY = wyBefore + (pos.y() - height() * 0.5) / scale;
    clampView();
    update();
}

ScenePainter::Drag ScenePainter::hitTest(const QPoint& pos, bool preferPolygon, int* corner) const {
    const double wx = worldX(pos.x());
    const double wy = worldY(pos.y());

    if (hasClipRect) {
        double tol = 6.0 / scale;
        const double minSide = std::min(clipRect.xmax - clipRect.xmin, clipRect.ymax - clipRect.ymin);
        if (minSide > 0.0) tol = std::min(tol, minSide * 0.3);

        const bool nearL = std::fabs(wx - clipRect.xmin) <= tol;
        const bool nearR = std::fabs(wx - clipRect.xmax) <= tol;
        const bool nearB = std::fabs(wy - clipRect.ymin) <= tol;
        const bool nearT = std::fabs(wy - clipRect.ymax) <= tol;

        int c = -1;
        if (nearL && nearB) c = 0;
        else if (nearR && nearB) c = 1;
        else if (nearL && nearT) c = 2;
        else if (nearR && nearT) c = 3;
        if (c >= 0) { if (corner) *corner = c; return Drag::ResizeRect; }
    }

    const bool insideRect = hasClipRect && wx > clipRect.xmin && wx < clipRect.xmax && wy > clipRect.ymin && wy < clipRect.ymax;
    const bool insidePoly = hasClipPolygon && showPolygons && clipPolygon.size() >= 3 && WeilerAtherton::pointInPolygon(clipPolygon, Point{ wx, wy });

    if (preferPolygon) {
        if (insidePoly) return Drag::MovePoly;
        if (insideRect) return Drag::MoveRect;
    } else {
        if (insideRect) return Drag::MoveRect;
        if (insidePoly) return Drag::MovePoly;
    }
    return Drag::Pan;
}

void ScenePainter::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        const bool preferPoly = event->modifiers().testFlag(Qt::ShiftModifier);
        int corner = 0;
        dragMode = hitTest(event->pos(), preferPoly, &corner);
        resizeCorner = corner;
        lastMousePos = event->pos();
        grabX = worldX(event->pos().x());
        grabY = worldY(event->pos().y());
        dragStartRect = clipRect;
        dragStartPolygon = clipPolygon;
    } else if (event->button() == Qt::MiddleButton) {
        dragMode = Drag::Pan;
        lastMousePos = event->pos();
    }
}

void ScenePainter::mouseMoveEvent(QMouseEvent* event) {
    if (dragMode == Drag::Pan) {
        const QPoint delta = event->pos() - lastMousePos;
        lastMousePos = event->pos();
        centerX -= delta.x() / scale;
        centerY += delta.y() / scale;
        clampView();
        update();
        return;
    }
    applyDrag(event->pos());
}

void ScenePainter::applyDrag(const QPoint& pos) {
    const double limit = WORLD_LIMIT;
    const double dx = worldX(pos.x()) - grabX;
    const double dy = worldY(pos.y()) - grabY;

    if (dragMode == Drag::MovePoly) {
        const Rect b = polygonBounds(dragStartPolygon);
        const double sx = (b.xmax - b.xmin <= 2.0 * limit)
                              ? std::clamp(dx, -limit - b.xmin, limit - b.xmax) : 0.0;
        const double sy = (b.ymax - b.ymin <= 2.0 * limit)
                              ? std::clamp(dy, -limit - b.ymin, limit - b.ymax) : 0.0;
        QVector<Point> moved;
        moved.reserve(dragStartPolygon.size());
        for (const Point& p : dragStartPolygon) moved.push_back(Point{ p.x + sx, p.y + sy });
        clipPolygon = moved;
        update();
        emit clipPolygonChanged(clipPolygon);
        return;
    }

    if (dragMode == Drag::MoveRect) {
        const double w = dragStartRect.xmax - dragStartRect.xmin;
        const double h = dragStartRect.ymax - dragStartRect.ymin;
        const double sx = (w <= 2.0 * limit)
                              ? std::clamp(dx, -limit - dragStartRect.xmin, limit - dragStartRect.xmax) : 0.0;
        const double sy = (h <= 2.0 * limit)
                              ? std::clamp(dy, -limit - dragStartRect.ymin, limit - dragStartRect.ymax) : 0.0;
        Rect r{ dragStartRect.xmin + sx, dragStartRect.ymin + sy,
               dragStartRect.xmax + sx, dragStartRect.ymax + sy };
        if (!sameRect(r, clipRect)) {
            clipRect = r;
            update();
            emit clipRectChanged(clipRect);
        }
        return;
    }

    if (dragMode == Drag::ResizeRect) {
        const bool dragMaxX = (resizeCorner == 1 || resizeCorner == 3);
        const bool dragMaxY = (resizeCorner == 2 || resizeCorner == 3);
        const double anchorX = dragMaxX ? dragStartRect.xmin : dragStartRect.xmax;
        const double anchorY = dragMaxY ? dragStartRect.ymin : dragStartRect.ymax;

        double mx = worldX(pos.x());
        double my = worldY(pos.y());
        mx = dragMaxX ? std::clamp(mx, std::min(anchorX + MIN_CLIP_SIZE, limit), limit)
                      : std::clamp(mx, -limit, std::max(anchorX - MIN_CLIP_SIZE, -limit));
        my = dragMaxY ? std::clamp(my, std::min(anchorY + MIN_CLIP_SIZE, limit), limit)
                      : std::clamp(my, -limit, std::max(anchorY - MIN_CLIP_SIZE, -limit));

        Rect r{ std::min(anchorX, mx), std::min(anchorY, my),
               std::max(anchorX, mx), std::max(anchorY, my) };
        if (!sameRect(r, clipRect)) {
            clipRect = r;
            update();
            emit clipRectChanged(clipRect);
        }
    }
}

void ScenePainter::mouseReleaseEvent(QMouseEvent*) { dragMode = Drag::None; }
void ScenePainter::resizeEvent(QResizeEvent*) { clampView(); }

void ScenePainter::drawAxes(QPainter& painter) {
    const double limit = WORLD_LIMIT;

    painter.setPen(QPen(QColor(170, 170, 170), 1, Qt::DashLine));
    painter.drawRect(QRectF(screenX(-limit), screenY(limit), 2 * limit * scale, 2 * limit * scale));

    const double rawStep = 70.0 / scale;
    const double powerOfTen = std::pow(10.0, std::floor(std::log10(rawStep)));
    double step = powerOfTen * 10;
    for (double m : { 1.0, 2.0, 5.0, 10.0 })
        if (powerOfTen * m >= rawStep) { step = powerOfTen * m; break; }
    const int decimals = std::max(0, -int(std::floor(std::log10(step))));

    const double left = worldX(0), right = worldX(width());
    const double top = worldY(0), bottom = worldY(height());

    painter.setPen(QPen(QColor(225, 225, 225), 1));
    for (double x = std::ceil(left / step) * step; x <= right; x += step)
        painter.drawLine(QLineF(screenX(x), 0, screenX(x), height()));
    for (double y = std::ceil(bottom / step) * step; y <= top; y += step)
        painter.drawLine(QLineF(0, screenY(y), width(), screenY(y)));

    painter.setPen(QPen(QColor(140, 140, 140), 1));
    if (left <= 0 && 0 <= right) painter.drawLine(QLineF(screenX(0), 0, screenX(0), height()));
    if (bottom <= 0 && 0 <= top) painter.drawLine(QLineF(0, screenY(0), width(), screenY(0)));

    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    painter.setPen(QColor(90, 90, 90));
    for (double x = std::ceil(left / step) * step; x <= right; x += step)
        painter.drawText(QRectF(screenX(x) + 2, height() - 16, 70, 14), Qt::AlignLeft, QString::number(x, 'f', decimals));
    for (double y = std::ceil(bottom / step) * step; y <= top; y += step)
        painter.drawText(QRectF(4, screenY(y) - 14, 70, 14), Qt::AlignLeft,QString::number(y, 'f', decimals));
}

void ScenePainter::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), Qt::white);
    drawAxes(painter);

    if (showQuadTree) {
        painter.setPen(QPen(QColor(190, 190, 190), 3, Qt::DotLine));
        for (const Rect& r : quadTreeRects)
            painter.drawRect(QRectF(screenX(r.xmin), screenY(r.ymax),
                                    (r.xmax - r.xmin) * scale, (r.ymax - r.ymin) * scale));
    }

    if (showPolygons) {
        painter.setPen(QPen(QColor(200, 60, 200), 2));
        for (const PolygonContour& c : originalPolygons) {
            if (c.pts.size() < 2) continue;
            for (int i = 0; i < c.pts.size(); ++i) {
                const Point& a = c.pts[i];
                const Point& b = c.pts[(i + 1) % c.pts.size()];
                painter.drawLine(QPointF(screenX(a.x), screenY(a.y)), QPointF(screenX(b.x), screenY(b.y)));
            }
        }
    }

    if (showOriginal) {
        painter.setPen(QPen(QColor(30, 100, 220), 2));
        for (const Segment& s : originalSegments)
            painter.drawLine(QPointF(screenX(s.p1.x), screenY(s.p1.y)), QPointF(screenX(s.p2.x), screenY(s.p2.y)));
    }

    if (showClipped) {
        painter.setPen(QPen(QColor(0, 150, 60), 3));
        for (const Segment& s : clippedSegments)
            painter.drawLine(QPointF(screenX(s.p1.x), screenY(s.p1.y)), QPointF(screenX(s.p2.x), screenY(s.p2.y)));
    }

    if (showPolygons) {
        QPainterPath path;
        for (const PolygonContour& c : clippedPolygons) {
            if (c.pts.size() < 3) continue;
            QPainterPath sub;
            sub.moveTo(screenX(c.pts[0].x), screenY(c.pts[0].y));
            for (int i = 1; i < c.pts.size(); ++i) sub.lineTo(screenX(c.pts[i].x), screenY(c.pts[i].y));
            sub.closeSubpath();
            path.addPath(sub);
        }
        if (!path.isEmpty()) {
            painter.fillPath(path, QBrush(QColor(0, 150, 60, 50)));
            painter.setPen(QPen(QColor(0, 150, 60), 2));
            painter.drawPath(path);
        }
    }

    if (hasClipPolygon && showPolygons && clipPolygon.size() >= 3){
        QPainterPath clipperPath;
        clipperPath.moveTo(screenX(clipPolygon[0].x), screenY(clipPolygon[0].y));
        for (int i = 1; i < clipPolygon.size(); ++i)
            clipperPath.lineTo(screenX(clipPolygon[i].x), screenY(clipPolygon[i].y));
        clipperPath.closeSubpath();
        painter.fillPath(clipperPath, QBrush(QColor(230, 120, 20, 18)));
        painter.setPen(QPen(QColor(230, 120, 20), 4));
        painter.drawPath(clipperPath);
    }

    if (hasClipRect) {
        painter.setPen(QPen(QColor(220, 40, 40), 5));
        painter.drawRect(QRectF(screenX(clipRect.xmin), screenY(clipRect.ymax), (clipRect.xmax - clipRect.xmin) * scale, (clipRect.ymax - clipRect.ymin) * scale));
    }
}