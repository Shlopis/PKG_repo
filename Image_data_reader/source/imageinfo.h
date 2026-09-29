#ifndef IMAGEINFO_H
#define IMAGEINFO_H

#include <QString>

struct ImageInfo {
    QString fileName;
    QString filePath;
    int width = 0;
    int height = 0;
    double dpiX = 0.0;
    double dpiY = 0.0;
    QString colorDepth;
    QString compression;
    QString status;
    QString errorText;
};

#endif // IMAGEINFO_H
