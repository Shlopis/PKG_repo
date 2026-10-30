#include "gridpainter.h"
#include <QPainter>
#include <QWheelEvent>

static const int MARGIN_LEFT   = 42;
static const int MARGIN_BOTTOM = 28;

GridPainter::GridPainter(QWidget* parent) : QWidget(parent) {
    setMinimumSize(400, 400);
}

void GridPainter::setGrid(const VirtualGrid* grid) { main_grid = grid; update(); }
void GridPainter::setCompareGrid(const VirtualGrid* other) { comp_grid = other; update(); }
void GridPainter::zoomIn()  { cell_size = std::clamp(cell_size + 2, 4, 40); update(); }
void GridPainter::zoomOut() { cell_size = std::clamp(cell_size - 2, 4, 40); update(); }

void GridPainter::wheelEvent(QWheelEvent* e) {
    if (e->angleDelta().y() > 0) zoomIn(); else zoomOut();
}

// Клетка (gx, gy) -верхний левый угол на экране
QPoint GridPainter::cellTopLeft(int gx, int gy) const {
    int x = MARGIN_LEFT + gx * cell_size;
    int y = height() - MARGIN_BOTTOM - (gy + 1) * cell_size;
    return QPoint(x, y);
}

void GridPainter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(30, 30, 30));
    if (!main_grid) return;
    drawPixels(p);
    drawGridAndAxes(p);
}

void GridPainter::drawPixels(QPainter& p) {
    for (int gy = 0; gy < main_grid->height(); ++gy)
        for (int gx = 0; gx < main_grid->width(); ++gx) {
            int mx = main_grid->minX() + gx; // индекс - математический x
            int my = main_grid->minY() + gy;  // индекс - математический y
            int v = main_grid->at(mx, my);
            if (v <= 0) continue;

            QColor c(255, 255, 255, v);
            if (comp_grid && comp_grid->at(mx, my) <= 0)
                c = QColor(255, 140, 0, v);

            QPoint tl = cellTopLeft(gx, gy);
            p.fillRect(tl.x(), tl.y(), cell_size, cell_size, c);
        }
}

void GridPainter::drawGridAndAxes(QPainter& p) {
    const int gw = main_grid->width()  * cell_size;
    const int gh = main_grid->height() * cell_size;
    const int ox = MARGIN_LEFT;
    const int oy = height() - MARGIN_BOTTOM;

    // Тонкая сетка по границам клеток
    if (cell_size >= 6) {
        p.setPen(QPen(QColor(70, 70, 70, 120), 1));
        for (int gx = 0; gx <= main_grid->width(); ++gx)
            p.drawLine(ox + gx * cell_size, oy, ox + gx * cell_size, oy - gh);
        for (int gy = 0; gy <= main_grid->height(); ++gy)
            p.drawLine(ox, oy - gy * cell_size, ox + gw, oy - gy * cell_size);
    }

    // ОСИ через математический ноль (центр поля при симметричном диапазоне)
    const int axisX = ox + (0 - main_grid->minX()) * cell_size;   // вертикаль x = 0
    const int axisY = oy - (0 - main_grid->minY()) * cell_size;   // горизонталь y = 0

    p.setPen(QPen(QColor(170, 170, 170), 2));
    p.drawLine(ox, axisY, ox + gw, axisY);  // ось X
    p.drawLine(axisX, oy, axisX, oy - gh); // ось Y

    // Подписи вдоль осей с адаптивным шагом
    int step = (cell_size >= 10) ? 5 : (cell_size >= 6 ? 10 : 20);
    QFont f = p.font(); f.setPointSize(8); p.setFont(f);
    p.setPen(QColor(210, 210, 210));

    for (int gx = 0; gx <= main_grid->width(); gx += step) {
        int value = main_grid->minX() + gx;
        if (value == 0) continue;
        p.drawText(QRect(ox + gx * cell_size - 15, axisY + 3, 30, 16), Qt::AlignHCenter | Qt::AlignTop, QString::number(value));
    }
    for (int gy = 0; gy <= main_grid->height(); gy += step) {
        int value = main_grid->minY() + gy;
        if (value == 0) continue;
        p.drawText(QRect(axisX - 36, oy - gy * cell_size - 8, 30, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(value));
    }
    p.drawText(QRect(axisX - 36, axisY + 3, 30, 16),  Qt::AlignRight | Qt::AlignTop, "0");
}


