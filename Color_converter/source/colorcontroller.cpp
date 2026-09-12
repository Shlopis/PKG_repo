#include "colorcontroller.h"
#include "mainwindow.h"

ColorController::ColorController(MainWindow* view)
    : QObject(view), view(view) {
}

void ColorController::updateFromRGB(int r, int g, int b) {
    RGB rgb = {r, g, b};

    LAB lab = ColorModel::rgbToLab(rgb);
    CMYK cmyk = ColorModel::labToCmyk(lab);

    view->updateRGB(r, g, b);
    view->updateLAB(lab.l, lab.a, lab.b);
    view->updateCMYK(cmyk.c * 100, cmyk.m * 100, cmyk.y * 100, cmyk.k * 100);
    view->updateColor(r, g, b);
}

void ColorController::updateFromLAB(double l, double a, double b) {
    LAB lab = {l, a, b};

    RGB rgb = ColorModel::labToRgb(lab);
    CMYK cmyk = ColorModel::labToCmyk(lab);

    view->updateRGB(rgb.r, rgb.g, rgb.b);
    view->updateLAB(l, a, b);
    view->updateCMYK(cmyk.c * 100, cmyk.m * 100, cmyk.y * 100, cmyk.k * 100);
    view->updateColor(rgb.r, rgb.g, rgb.b);
}

void ColorController::updateFromCMYK(double c, double m, double y, double k) {
    CMYK cmyk = {c / 100.0, m / 100.0, y / 100.0, k / 100.0};

    LAB lab = ColorModel::cmykToLab(cmyk);
    RGB rgb = ColorModel::labToRgb(lab);

    view->updateRGB(rgb.r, rgb.g, rgb.b);
    view->updateLAB(lab.l, lab.a, lab.b);
    view->updateCMYK(c, m, y, k);
    view->updateColor(rgb.r, rgb.g, rgb.b);
}

void ColorController::changeStandart(int index) {
    ColorModel::setStandart(index);

    int r, g, b;
    view->getCurrentRGB(r, g, b);
    updateFromRGB(r, g, b);
}