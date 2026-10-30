#ifndef GRIDPAINTER_H
#define GRIDPAINTER_H

#include <QWidget>
#include "virtualgrid.h"

class GridPainter : public QWidget {
    Q_OBJECT
public:
    explicit GridPainter(QWidget* parent = nullptr);

    void setGrid(const VirtualGrid* grid);
    void setCompareGrid(const VirtualGrid* other);
    void zoomIn();
    void zoomOut();

protected:
    void paintEvent(QPaintEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

private:
    const VirtualGrid* main_grid = nullptr;
    const VirtualGrid* comp_grid = nullptr;
    int cell_size = 8;

    QPoint cellTopLeft(int gx, int gy) const; // gx, gy
    void drawPixels(QPainter& p);
    void drawGridAndAxes(QPainter& p);
};

#endif // GRIDPAINTER_H
