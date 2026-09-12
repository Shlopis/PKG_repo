#ifndef COLORMODEL_H
#define COLORMODEL_H

//#include <QString>

struct RGB {
    int r, g, b;
};

struct LAB {
    double l, a, b;
};

struct CMYK {
    double c, m, y, k;
};

class ColorModel {
public:
    static void setStandart(int index);
    static int getStandart();

    static LAB rgbToLab(const RGB& rgb);
    static RGB labToRgb(const LAB& lab);
    static LAB cmykToLab(const CMYK& cmyk);
    static CMYK labToCmyk(const LAB& lab);

private:
    static int standart;
    static double whiteX, whiteY, whiteZ;

    static double rgbToXyz_D65[3][3];
    static double xyzToRgb_D65[3][3];
    static double rgbToXyz_D50[3][3];
    static double xyzToRgb_D50[3][3];
    static double rgbToXyz_E[3][3];
    static double xyzToRgb_E[3][3];

    static double (*currentRgbToXyz)[3];
    static double (*currentXyzToRgb)[3];

    static double labF(double t);
    static double labInvF(double t);
    static double gammaCorrect(double c);
    static double gammaUncorrect(double c);
};
#endif // COLORMODEL_H