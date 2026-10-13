#ifndef IMAGEWRAPPER_H
#define IMAGEWRAPPER_H

#include <QImage>
#include <QString>
#include <stdexcept>

class ImageWrapper {
private:
    QImage image;
    int img_width;
    int img_height;
    bool img_isGray;

public:
    ImageWrapper() : img_width(0), img_height(0), img_isGray(false) {}

    bool loadImage(const QString& filePath) {
        QImage img(filePath);
        if (img.isNull()) return false;

        if (img.format() == QImage::Format_Grayscale8 || img.format() == QImage::Format_Indexed8) {
            image = img.convertToFormat(QImage::Format_Grayscale8);
            img_isGray = true;
        } else {
            image = img.convertToFormat(QImage::Format_RGB32);
            img_isGray = false;
        }
        img_width = image.width();
        img_height = image.height();
        return (img_width > 0 && img_height > 0);
    }

    void ensureDetached() { image.detach(); }

    const uchar* getGrayScanLineConst(int y) const {
        if (!img_isGray) throw std::runtime_error("Not grayscale");
        return image.constScanLine(y);
    }

    const QRgb* getRgbScanLineConst(int y) const {
        if (img_isGray) throw std::runtime_error("Not RGB");
        return reinterpret_cast<const QRgb*>(image.constScanLine(y));
    }

    uchar* getGrayScanLineMutable(int y) {
        if (!img_isGray) throw std::runtime_error("Not grayscale");
        return image.scanLine(y);
    }

    QRgb* getRgbScanLineMutable(int y) {
        if (img_isGray) throw std::runtime_error("Not RGB");
        return reinterpret_cast<QRgb*>(image.scanLine(y));
    }

    int width() const { return img_width; }
    int height() const { return img_height; }
    bool isGrayscale() const { return img_isGray; }
    const QImage& qImage() const { return image; }
};

#endif // IMAGEWRAPPER_H
