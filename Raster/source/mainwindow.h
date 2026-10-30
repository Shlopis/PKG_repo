#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include "virtualgrid.h"
#include "gridpainter.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

private slots:
    void onDraw();
    void onRandom();
    void onClear();
    void onBenchmark();

private:
    VirtualGrid gridA, gridB;
    GridPainter *painterA, *painterB;
    QComboBox *comboA, *comboB;
    QSpinBox *s_x0, *s_y0, *s_x1, *s_y1;
    QLabel *diffLabel;
};

#endif // MAINWINDOW_H
