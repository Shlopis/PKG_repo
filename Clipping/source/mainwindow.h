#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCheckBox>
#include <QComboBox>
#include "geom.h"
#include "quadtree.h"
#include "scenepainter.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

private slots:
    void onLoad();
    void onFit();
    void onPolygonChanged();
    void onStress();

private:
    ScenePainter* scene = nullptr;
    QCheckBox* originalCheckbox = nullptr;
    QCheckBox* clippedCheckbox = nullptr;
    QCheckBox* quadTreeCheckbox = nullptr;
    QComboBox* polygonCombo = nullptr;

    QVector<Segment> segments;
    QVector<Segment> clippedSegments;
    QVector<PolygonContour> polygons;
    QVector<PolygonContour> clippedPolygons;
    QuadTree quadTree;

    Rect clipRect;
    bool hasClipRect = false;

    QVector<Point> polygonClipper;

    Rect currentWindow() const;
    void runClipping();
    void updatePolygons();
};

#endif // MAINWINDOW_H