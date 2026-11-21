#ifndef SCENEPAINTER_H
#define SCENEPAINTER_H

#include <QWidget>
#include "geom.h"

class ScenePainter : public QWidget {
    Q_OBJECT
public:
    static constexpr double WORLD_LIMIT = 100.0;

    explicit ScenePainter(QWidget* parent = nullptr);

    void setSegments(const QVector<Segment>& original, const QVector<Segment>& clipped);
    void setClipRect(const Rect& rect, bool has);                 // отсекатель отрезков
    void setClipPolygon(const QVector<Point>& polygon, bool has); // отсекатель полигонов
    void setQuadTreeRects(const QVector<Rect>& rects);
    void setPolygons(const QVector<PolygonContour>& original, const QVector<PolygonContour>& clipped);
    void setFlags(bool showOriginal, bool showClipped, bool showQuadTree, bool showPolygons);

    void fitToContent();

signals:
    void clipRectChanged(const Rect& rect);
    void clipPolygonChanged(const QVector<Point>& polygon);

protected:
    void paintEvent(QPaintEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    // Pan — панорама; MoveRect/MovePoly — перенос отсекателей; ResizeRect — только за угол
    enum class Drag { None, Pan, MoveRect, MovePoly, ResizeRect };

    QVector<Segment> originalSegments, clippedSegments;
    QVector<PolygonContour> originalPolygons, clippedPolygons;
    QVector<Rect> quadTreeRects;

    Rect clipRect;
    bool hasClipRect = false;
    QVector<Point> clipPolygon;
    bool hasClipPolygon = false;

    bool showOriginal = true;
    bool showClipped = true;
    bool showQuadTree = false;
    bool showPolygons = true;

    double scale = 4;
    double centerX = 0;
    double centerY = 0;

    // Состояние перетаскивания: снимки на момент нажатия (без накопления ошибки)
    Drag dragMode = Drag::None;
    QPoint lastMousePos;
    double grabX = 0, grabY = 0;        // мировая точка захвата
    Rect dragStartRect;                 // снимок прямоугольника
    QVector<Point> dragStartPolygon;    // снимок вогнутого отсекателя
    int resizeCorner = 0;               // 0=BL, 1=BR, 2=TL, 3=TR

    double screenX(double x) const { return width() * 0.5 + (x - centerX) * scale; }
    double screenY(double y) const { return height() * 0.5 - (y - centerY) * scale; }
    double worldX(double px) const { return centerX + (px - width() * 0.5) / scale; }
    double worldY(double py) const { return centerY - (py - height() * 0.5) / scale; }

    void clampView();
    Drag hitTest(const QPoint& pos, bool preferPolygon, int* corner = nullptr) const;
    void applyDrag(const QPoint& pos);
    void updateHoverCursor(const QPoint& pos);
    void drawAxes(QPainter& painter);
};

#endif // SCENEPAINTER_H