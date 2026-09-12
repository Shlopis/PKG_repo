#ifndef COLORCONTROLLER_H
#define COLORCONTROLLER_H

#include <QObject>
#include "colormodel.h"

class MainWindow;

class ColorController : public QObject {
    Q_OBJECT

public:
    ColorController(MainWindow* view);

    void updateFromRGB(int r, int g, int b);
    void updateFromLAB(double L, double a, double b);
    void updateFromCMYK(double c, double m, double y, double k);
    void changeStandart(int index);

private:
    MainWindow* view;
};
#endif // COLORCONTROLLER_H
