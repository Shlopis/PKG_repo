#include "colormodel.h"
#include <cmath>
#include <algorithm>

int ColorModel::standart = 0;
double ColorModel::whiteX = 0.95047;
double ColorModel::whiteY = 1.0;
double ColorModel::whiteZ = 1.08883;

double (*ColorModel::currentRgbToXyz)[3] = ColorModel::rgbToXyz_D65;
double (*ColorModel::currentXyzToRgb)[3] = ColorModel::xyzToRgb_D65;

double ColorModel::rgbToXyz_D65[3][3] = {
    {0.412453, 0.357580, 0.180423},
    {0.212671, 0.715160, 0.072169},
    {0.019334, 0.119193, 0.950227}
};

double ColorModel::xyzToRgb_D65[3][3] = {
    {3.2406, -1.5372, -0.4986},
    {-0.9689, 1.8758, 0.0415},
    {0.0557, -0.2040, 1.0570}
};

double ColorModel::rgbToXyz_D50[3][3] = {
    {0.4360747, 0.3850649, 0.1430804},
    {0.2225045, 0.7168786, 0.0606169},
    {0.0139322, 0.0971045, 0.7141733}
};

double ColorModel::xyzToRgb_D50[3][3] = {
    {3.1338561, -1.6168667, -0.4906146},
    {-0.9787684, 1.9161415, 0.0334540},
    {0.0719453, -0.2289914, 1.4052427}
};

double ColorModel::rgbToXyz_E[3][3] = {
    {0.412453, 0.357580, 0.180423},
    {0.212671, 0.715160, 0.072169},
    {0.019334, 0.119193, 0.950227}
};

double ColorModel::xyzToRgb_E[3][3] = {
    {3.2406, -1.5372, -0.4986},
    {-0.9689, 1.8758, 0.0415},
    {0.0557, -0.2040, 1.0570}
};

void ColorModel::setStandart(int st) {
    standart = st;

    switch(st) {
    case 0:// D65
        whiteX = 0.95047;
        whiteY = 1.0;
        whiteZ = 1.08883;
        currentRgbToXyz = rgbToXyz_D65;
        currentXyzToRgb = xyzToRgb_D65;
        break;
    case 1:// D50
        whiteX = 0.96421;
        whiteY = 1.0;
        whiteZ = 0.82519;
        currentRgbToXyz = rgbToXyz_D50;
        currentXyzToRgb = xyzToRgb_D50;
        break;
    case 2:// E
        whiteX = 1.0;
        whiteY = 1.0;
        whiteZ = 1.0;
        currentRgbToXyz = rgbToXyz_E;
        currentXyzToRgb = xyzToRgb_E;
        break;
    }
}

int ColorModel::getStandart() {
    return standart;
}

double ColorModel::labF(double t) {
    const double delta = 6.0 / 29.0;
    if (t > delta * delta * delta) return std::cbrt(t);
    return t / (3 * delta * delta) + 4.0 / 29.0;
}

double ColorModel::labInvF(double t) {
    const double delta = 6.0 / 29.0;
    if (t > delta) return t * t * t;
    return 3 * delta * delta * (t - 4.0 / 29.0);
}

double ColorModel::gammaCorrect(double c) {
    if (c <= 0.0031308) return c * 12.92;
    return 1.055 * std::pow(c, 1.0/2.4) - 0.055;
}

double ColorModel::gammaUncorrect(double c) {
    if (c <= 0.04045) return c / 12.92;
    return std::pow((c + 0.055) / 1.055, 2.4);
}

LAB ColorModel::rgbToLab(const RGB& rgb) {
    double r = rgb.r / 255.0;
    double g = rgb.g / 255.0;
    double b = rgb.b / 255.0;

    double R = gammaUncorrect(r);
    double G = gammaUncorrect(g);
    double B = gammaUncorrect(b);

    double X = currentRgbToXyz[0][0]*R + currentRgbToXyz[0][1]*G + currentRgbToXyz[0][2]*B;
    double Y = currentRgbToXyz[1][0]*R + currentRgbToXyz[1][1]*G + currentRgbToXyz[1][2]*B;
    double Z = currentRgbToXyz[2][0]*R + currentRgbToXyz[2][1]*G + currentRgbToXyz[2][2]*B;

    X /= whiteX;
    Y /= whiteY;
    Z /= whiteZ;

    double fx = labF(X);
    double fy = labF(Y);
    double fz = labF(Z);

    LAB lab;
    lab.l = 116 * fy - 16;
    lab.a = 500 * (fx - fy);
    lab.b = 200 * (fy - fz);

    return lab;
}

RGB ColorModel::labToRgb(const LAB& lab) {
    double fy = (lab.l + 16) / 116;
    double fx = fy + lab.a / 500;
    double fz = fy - lab.b / 200;

    double X = labInvF(fx) * whiteX;
    double Y = labInvF(fy) * whiteY;
    double Z = labInvF(fz) * whiteZ;

    double R = currentXyzToRgb[0][0]*X + currentXyzToRgb[0][1]*Y + currentXyzToRgb[0][2]*Z;
    double G = currentXyzToRgb[1][0]*X + currentXyzToRgb[1][1]*Y + currentXyzToRgb[1][2]*Z;
    double B = currentXyzToRgb[2][0]*X + currentXyzToRgb[2][1]*Y + currentXyzToRgb[2][2]*Z;

    double r = gammaCorrect(R);
    double g = gammaCorrect(G);
    double b = gammaCorrect(B);

    RGB rgb;
    rgb.r = round(std::max(0.0, std::min(1.0, r)) * 255);
    rgb.g = round(std::max(0.0, std::min(1.0, g)) * 255);
    rgb.b = round(std::max(0.0, std::min(1.0, b)) * 255);

    return rgb;
}

LAB ColorModel::cmykToLab(const CMYK& cmyk) {
    double r = (1 - cmyk.c) * (1 - cmyk.k);
    double g = (1 - cmyk.m) * (1 - cmyk.k);
    double b = (1 - cmyk.y) * (1 - cmyk.k);

    RGB rgb;
    rgb.r = round(r * 255);
    rgb.g = round(g * 255);
    rgb.b = round(b * 255);

    return rgbToLab(rgb);
}

CMYK ColorModel::labToCmyk(const LAB& lab) {
    RGB rgb = labToRgb(lab);

    double r = rgb.r / 255.0;
    double g = rgb.g / 255.0;
    double b = rgb.b / 255.0;

    double k = 1 - std::max({r, g, b});

    CMYK cmyk;
    if (k >= 1) {
        cmyk.c = 0; cmyk.m = 0; cmyk.y = 0; cmyk.k = 1;
    } else {
        cmyk.c = (1 - r - k) / (1 - k);
        cmyk.m = (1 - g - k) / (1 - k);
        cmyk.y = (1 - b - k) / (1 - k);
        cmyk.k = k;
    }

    return cmyk;
}