#include "Parsers.h"

#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QVector>


//quint8 = unsigned char

quint16 ByteUtils::readU16LE(const QByteArray &b, int off)
{
    return quint16(quint8(b[off])) | (quint16(quint8(b[off + 1])) << 8);
}

quint16 ByteUtils::readU16BE(const QByteArray &b, int off)
{
    return (quint16(quint8(b[off])) << 8) |  quint16(quint8(b[off + 1]));
}

quint32 ByteUtils::readU32LE(const QByteArray &b, int off)
{
    return  quint32(quint8(b[off])) | (quint32(quint8(b[off + 1])) <<  8) | (quint32(quint8(b[off + 2])) << 16) | (quint32(quint8(b[off + 3])) << 24);
}

quint32 ByteUtils::readU32BE(const QByteArray &b, int off)
{
    return (quint32(quint8(b[off])) << 24) | (quint32(quint8(b[off + 1])) << 16) | (quint32(quint8(b[off + 2])) <<  8) |  quint32(quint8(b[off + 3]));
}

qint32 ByteUtils::readS32LE(const QByteArray &b, int off)
{
    return qint32(readU32LE(b, off));
}




static const QByteArray PngSignature = QByteArray::fromHex("89504E470D0A1A0A");

static int pngSamplesForColorType(int colorType)
{
    switch (colorType) {
    case 0: return 1;
    case 2: return 3;
    case 3: return 1;
    case 4: return 2;
    case 6: return 4;
    default: return 0;
    }
}

static QString pngDescribeColorDepth(int bitDepth, int colorType)
{
    const int samples = pngSamplesForColorType(colorType);
    if (samples == 0)
        return QStringLiteral("%1 бит (неизвестный тип цвета %2)").arg(bitDepth).arg(colorType);

    const int totalBits = bitDepth * samples;
    switch (colorType) {
    case 0: return QStringLiteral("%1 бит (оттенки серого)").arg(bitDepth);
    case 2: return QStringLiteral("%1 бит (RGB, %2 бит/канал)").arg(totalBits).arg(bitDepth);
    case 3: {
        const int paletteSize = 1 << bitDepth;
        return QStringLiteral("%1 бит (индексированный, палитра до %2 цветов)").arg(bitDepth).arg(paletteSize);
    }
    case 4: return QStringLiteral("%1 бит (серый + альфа)").arg(totalBits);
    case 6: return QStringLiteral("%1 бит (RGBA, %2 бит/канал)").arg(totalBits).arg(bitDepth);
    default: return QStringLiteral("%1 бит").arg(totalBits);
    }
}

bool PNGParser::canParse(const QString &filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    return f.read(8) == PngSignature;
}

ImageInfo PNGParser::parse(const QString &filePath) const
{
    ImageInfo info;
    info.fileName = QFileInfo(filePath).fileName();
    info.filePath = filePath;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Не удалось открыть файл");
        return info;
    }

    if (f.read(8) != PngSignature) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Неверная сигнатура PNG");
        return info;
    }

    const qint64 fileSize = f.size();
    bool gotIHDR = false;
    bool gotIEND = false;

    while (!f.atEnd()) {
        const QByteArray lenBytes = f.read(4);
        if (lenBytes.size() != 4) break;

        const quint32 length = ByteUtils::readU32BE(lenBytes);
        const QByteArray type = f.read(4);
        if (type.size() != 4) break;

        const qint64 dataStart = f.pos();
        const qint64 chunkEnd = dataStart + length + 4;

        if (chunkEnd > fileSize) {
            info.status = QStringLiteral("Повреждён");
            info.errorText = QStringLiteral("Чанк выходит за пределы файла");
            return info;
        }

        if (type == "IHDR") {
            if (length != 13) {
                info.status = QStringLiteral("Повреждён");
                info.errorText  = QStringLiteral("Некорректная длина IHDR: %1").arg(length);
                return info;
            }
            const QByteArray d = f.read(13);
            if (d.size() != 13) {
                info.status = QStringLiteral("Повреждён");
                info.errorText  = QStringLiteral("Обрыв данных IHDR");
                return info;
            }
            info.width = int(ByteUtils::readU32BE(d, 0));
            info.height = int(ByteUtils::readU32BE(d, 4));
            const int bitDepth = quint8(d[8]);
            const int colorType = quint8(d[9]);
            info.colorDepth = pngDescribeColorDepth(bitDepth, colorType);
            info.compression = QStringLiteral("Deflate (zlib)");
            gotIHDR = true;
        }
        else if (type == "pHYs") {
            if (length == 9) {
                const QByteArray d = f.read(9);
                if (d.size() == 9) {
                    const quint32 ppuX = ByteUtils::readU32BE(d, 0);
                    const quint32 ppuY = ByteUtils::readU32BE(d, 4);
                    const int unit = quint8(d[8]);
                    if (unit == 1) {
                        info.dpiX = ppuX * 0.0254;
                        info.dpiY = ppuY * 0.0254;
                    }
                }
            }
        }
        else if (type == "IEND") {
            gotIEND = true;
            break;
        }

        if (!f.seek(chunkEnd)) {
            info.status = QStringLiteral("Повреждён");
            info.errorText  = QStringLiteral("Не удалось перейти к следующему чанку");
            return info;
        }
    }

    if (!gotIHDR) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Отсутствует чанк IHDR");
        return info;
    }
    if (!gotIEND) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Отсутствует чанк IEND в конце файла");
        return info;
    }

    info.status = QStringLiteral("OK");
    return info;
}




static QString bmpDescribeCompression(quint32 code)
{
    switch (code) {
    case 0: return QStringLiteral("BI_RGB (без сжатия)");
    case 1: return QStringLiteral("BI_RLE8 (RLE, 8 бит)");
    case 2: return QStringLiteral("BI_RLE4 (RLE, 4 бита)");
    case 3: return QStringLiteral("BI_BITFIELDS (битовые маски)");
    case 4: return QStringLiteral("BI_JPEG (встроенный JPEG)");
    case 5: return QStringLiteral("BI_PNG (встроенный PNG)");
    case 6: return QStringLiteral("BI_ALPHABITFIELDS (маски + альфа)");
    default: return QStringLiteral("Неизвестный код сжатия (%1)").arg(code);
    }
}

static QString bmpDescribeColorDepth(quint16 bitCount, quint32 compression)
{
    QString base;
    switch (bitCount) {
    case 1: base = QStringLiteral("1 бит (монохром + палитра)"); break;
    case 4: base = QStringLiteral("4 бита (индексированный, 16 цветов)"); break;
    case 8: base = QStringLiteral("8 бит (индексированный, 256 цветов)"); break;
    case 16: base = QStringLiteral("16 бит (High Color)"); break;
    case 24: base = QStringLiteral("24 бита (True Color, RGB)"); break;
    case 32: base = QStringLiteral("32 бита (True Color + альфа)"); break;
    default: base = QStringLiteral("%1 бит").arg(bitCount);
    }
    if (compression == 3 || compression == 6)
        base += QStringLiteral(", маски заданы");
    return base;
}

bool BMPParser::canParse(const QString &filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    return f.read(2) == QByteArray("BM");
}

ImageInfo BMPParser::parse(const QString &filePath) const
{
    ImageInfo info;
    info.fileName = QFileInfo(filePath).fileName();
    info.filePath = filePath;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Не удалось открыть файл");
        return info;
    }

    const qint64 fileSize = f.size();

    const QByteArray fh = f.read(14);
    if (fh.size() != 14) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Обрыв BITMAPFILEHEADER");
        return info;
    }
    if (fh.left(2) != QByteArrayLiteral("BM")) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Неверная сигнатура BMP");
        return info;
    }

    const quint32 bfSize = ByteUtils::readU32LE(fh, 2);
    const quint32 bfOffBits = ByteUtils::readU32LE(fh, 10);

    if (bfSize != 0 && qint64(bfSize) > fileSize) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Размер в заголовке больше файла");
        return info;
    }

    const QByteArray sizeBytes = f.read(4);
    if (sizeBytes.size() != 4) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Обрыв в BITMAPINFOHEADER");
        return info;
    }
    const quint32 biSize = ByteUtils::readU32LE(sizeBytes);

    if (biSize < 12 || biSize > 124) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Некорректный размер BITMAPINFOHEADER");
        return info;
    }
    if (14 + qint64(biSize) > fileSize) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("BITMAPINFOHEADER выходит за пределы файла");
        return info;
    }

    const QByteArray rest = f.read(int(biSize) - 4);
    if (rest.size() != int(biSize) - 4) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Обрыв BITMAPINFOHEADER");
        return info;
    }

    quint32 width = 0, height = 0;
    quint16 bitCount = 0;
    quint32 compression = 0;
    qint32  ppmX = 0, ppmY = 0;
    quint32 clrUsed = 0;

    if (biSize == 12) {
        width = ByteUtils::readU16LE(rest, 0);
        height = ByteUtils::readU16LE(rest, 2);
        bitCount = ByteUtils::readU16LE(rest, 6);
    } else {
        width = ByteUtils::readU32LE(rest, 0);
        height = ByteUtils::readU32LE(rest, 4);
        bitCount = ByteUtils::readU16LE(rest, 10);
        compression = ByteUtils::readU32LE(rest, 12);
        ppmX = ByteUtils::readS32LE(rest, 20);
        ppmY = ByteUtils::readS32LE(rest, 24);
        clrUsed = ByteUtils::readU32LE(rest, 28);
    }

    if (width == 0 || height == 0) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Нулевые размеры изображения");
        return info;
    }

    if (bitCount > 0 && bitCount <= 8) {
        const quint32 paletteEntries = (clrUsed != 0) ? clrUsed : (1u << bitCount);
        const qint64 paletteStart = 14 + biSize;
        const qint64 paletteBytes = (biSize == 12) ? qint64(paletteEntries) * 3 : qint64(paletteEntries) * 4;
        if (paletteStart + paletteBytes > fileSize) {
            info.status = QStringLiteral("Повреждён");
            info.errorText = QStringLiteral("Палитра выходит за пределы файла");
            return info;
        }
    }

    if (bfOffBits != 0 && qint64(bfOffBits) > fileSize) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Смещение пикселей больше файла");
        return info;
    }

    info.width = int(width);
    info.height = int(height);
    info.colorDepth = bmpDescribeColorDepth(bitCount, compression);
    info.compression = bmpDescribeCompression(compression);
    if (ppmX > 0) info.dpiX = ppmX * 0.0254;
    if (ppmY > 0) info.dpiY = ppmY * 0.0254;

    info.status = QStringLiteral("OK");
    return info;
}



static bool jpegIsSOFMarker(quint8 m)
{
    if (m < 0xC0 || m > 0xCF) return false;
    return m != 0xC4 && m != 0xC8 && m != 0xCC;
}

static QString jpegDescribeSOF(quint8 marker)
{
    switch (marker) {
    case 0xC0: return QStringLiteral("Lossy DCT (baseline)");
    case 0xC1: return QStringLiteral("Lossy DCT (extended sequential)");
    case 0xC2: return QStringLiteral("Lossy DCT (progressive)");
    case 0xC3: return QStringLiteral("Lossless (sequential)");
    case 0xC5: return QStringLiteral("Lossy DCT (differential sequential)");
    case 0xC6: return QStringLiteral("Lossy DCT (differential progressive)");
    case 0xC7: return QStringLiteral("Lossless (differential)");
    case 0xC9: return QStringLiteral("Arithmetic (extended sequential)");
    case 0xCA: return QStringLiteral("Arithmetic (progressive)");
    case 0xCB: return QStringLiteral("Arithmetic (lossless)");
    case 0xCD: return QStringLiteral("Arithmetic (differential sequential)");
    case 0xCE: return QStringLiteral("Arithmetic (differential progressive)");
    case 0xCF: return QStringLiteral("Arithmetic (differential lossless)");
    default:   return QStringLiteral("JPEG (неизвестный SOF)");
    }
}

static QString jpegDescribeColorDepth(int precision, int components)
{
    const int total = precision * components;
    switch (components) {
    case 1: return QStringLiteral("%1 бит (оттенки серого)").arg(precision);
    case 3: return QStringLiteral("%1 бит (YCbCr/RGB, %2 бит/канал)").arg(total).arg(precision);
    case 4: return QStringLiteral("%1 бит (CMYK, %2 бит/канал)").arg(total).arg(precision);
    default: return QStringLiteral("%1 бит (%2 канала)").arg(total).arg(components);
    }
}

bool JPEGParser::canParse(const QString &filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const QByteArray soi = f.read(2);
    return soi.size() == 2 && quint8(soi[0]) == 0xFF && quint8(soi[1]) == 0xD8;
}

ImageInfo JPEGParser::parse(const QString &filePath) const
{
    ImageInfo info;
    info.fileName = QFileInfo(filePath).fileName();
    info.filePath = filePath;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Не удалось открыть файл");
        return info;
    }

    const qint64 fileSize = f.size();
    if (fileSize < 4) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Файл слишком маленький");
        return info;
    }

    const QByteArray soi = f.read(2);
    if (soi.size() != 2 || quint8(soi[0]) != 0xFF || quint8(soi[1]) != 0xD8) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Неверная сигнатура JPEG (нет SOI)");
        return info;
    }

    bool gotSOF = false;

    while (!f.atEnd()) {
        const QByteArray markerBytes = f.read(2);
        if (markerBytes.size() != 2) break;

        quint8 m0 = quint8(markerBytes[0]);
        quint8 m1 = quint8(markerBytes[1]);

        if (m0 != 0xFF) {
            info.status = QStringLiteral("Повреждён");
            info.errorText = QStringLiteral("Потеряна синхронизация маркеров");
            return info;
        }

        while (m1 == 0xFF) {
            if (f.atEnd()) break;
            const QByteArray pad = f.read(1);
            if (pad.size() != 1) break;
            m1 = quint8(pad[0]);
        }

        if (m1 == 0xD9) break;
        if ((m1 >= 0xD0 && m1 <= 0xD7) || m1 == 0x01) continue;

        const QByteArray lenBytes = f.read(2);
        if (lenBytes.size() != 2) {
            info.status = QStringLiteral("Повреждён");
            info.errorText = QStringLiteral("Обрыв при чтении длины сегмента");
            return info;
        }
        const quint16 segLen = ByteUtils::readU16BE(lenBytes);
        if (segLen < 2) {
            info.status = QStringLiteral("Повреждён");
            info.errorText = QStringLiteral("Некорректная длина сегмента");
            return info;
        }

        const qint64 segDataStart = f.pos();
        const qint64 segDataLen = qint64(segLen) - 2;
        const qint64 segEnd = segDataStart + segDataLen;

        if (segEnd > fileSize) {
            info.status = QStringLiteral("Повреждён");
            info.errorText  = QStringLiteral("Сегмент выходит за пределы файла (маркер FF %1)");
            return info;
        }

        if (m1 == 0xDA) break;

        if (jpegIsSOFMarker(m1)) {
            const QByteArray d = f.read(int(segDataLen));
            if (d.size() != int(segDataLen) || d.size() < 6) {
                info.status = QStringLiteral("Повреждён");
                info.errorText = QStringLiteral("Обрыв в SOF-сегменте");
                return info;
            }
            const int precision = quint8(d[0]);
            const int height = ByteUtils::readU16BE(d, 1);
            const int width = ByteUtils::readU16BE(d, 3);
            const int components = quint8(d[5]);

            if (width <= 0 || height <= 0 || components <= 0) {
                info.status = QStringLiteral("Повреждён");
                info.errorText  = QStringLiteral("Некорректные размеры/компоненты в SOF");
                return info;
            }

            info.width = width;
            info.height = height;
            info.colorDepth = jpegDescribeColorDepth(precision, components);
            info.compression = jpegDescribeSOF(m1);
            gotSOF = true;
        }
        else if (m1 == 0xE0) {
            const QByteArray d = f.read(int(segDataLen));
            if (d.size() == int(segDataLen) && d.size() >= 12) {
                if (d.left(5) == QByteArrayLiteral("JFIF\0")) {
                    const quint8 units = quint8(d[7]);
                    const quint16 xd = ByteUtils::readU16BE(d, 8);
                    const quint16 yd = ByteUtils::readU16BE(d, 10);
                    if (units == 1) {
                        info.dpiX = xd;
                        info.dpiY = yd;
                    }
                    else if (units == 2) {
                        info.dpiX = xd * 2.54;
                        info.dpiY = yd * 2.54;
                    }
                }
            }
        }

        if (!f.seek(segEnd)) {
            info.status = QStringLiteral("Повреждён");
            info.errorText = QStringLiteral("Не удалось перейти к следующему маркеру");
            return info;
        }
    }

    if (!gotSOF) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Не найден маркер SOF (нет размера изображения)");
        return info;
    }

    if (!f.seek(fileSize - 2)) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Не удалось прочитать конец файла");
        return info;
    }
    const QByteArray eoi = f.read(2);
    if (eoi.size() != 2 || quint8(eoi[0]) != 0xFF || quint8(eoi[1]) != 0xD9) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Отсутствует маркер EOI в конце файла");
        return info;
    }

    info.status = QStringLiteral("OK");
    return info;
}



static bool gifSkipSubBlocks(QFile &f, qint64 fileSize)
{
    while (true) {
        const QByteArray lenByte = f.read(1);
        if (lenByte.size() != 1)
            return false;
        const quint8 n = quint8(lenByte[0]);
        if (n == 0)
            return true;
        const qint64 next = f.pos() + n;
        if (next > fileSize)
            return false;
        if (!f.seek(next))
            return false;
    }
}

static QString gifDescribeColorDepth(bool hasGCT, int packedByte)
{
    if (!hasGCT)
        return QStringLiteral("Индексированный (палитра в кадрах, до 8 бит/пиксель)");
    const int n = packedByte & 0x07;
    const int bitDepth = n + 1;
    const int colors = 1 << bitDepth;
    return QStringLiteral("%1 бит (индексированный, до %2 цветов)").arg(bitDepth).arg(colors);
}

bool GIFParser::canParse(const QString &filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const QByteArray sig = f.read(6);
    return sig == QByteArrayLiteral("GIF87a") || sig == QByteArrayLiteral("GIF89a");
}

ImageInfo GIFParser::parse(const QString &filePath) const
{
    ImageInfo info;
    info.fileName = QFileInfo(filePath).fileName();
    info.filePath = filePath;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Не удалось открыть файл");
        return info;
    }

    const qint64 fileSize = f.size();
    if (fileSize < 14) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Файл слишком маленький");
        return info;
    }

    const QByteArray header = f.read(6);
    if (header != QByteArrayLiteral("GIF87a")
        && header != QByteArrayLiteral("GIF89a")) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Неверная сигнатура GIF");
        return info;
    }

    const QByteArray lsd = f.read(7);
    if (lsd.size() != 7) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Обрыв Logical Screen Descriptor");
        return info;
    }
    const int width = ByteUtils::readU16LE(lsd, 0);
    const int height = ByteUtils::readU16LE(lsd, 2);
    const quint8 packed = quint8(lsd[4]);

    if (width <= 0 || height <= 0) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Нулевые размеры в LSD");
        return info;
    }

    const bool hasGCT = (packed & 0x80) != 0;

    if (hasGCT) {
        const int n = packed & 0x07;
        const qint64 gctSz = qint64(3) * (1 << (n + 1));
        const qint64 next = f.pos() + gctSz;
        if (next > fileSize) {
            info.status = QStringLiteral("Повреждён");
            info.errorText  = QStringLiteral("Global Color Table выходит за пределы файла");
            return info;
        }
        if (!f.seek(next)) {
            info.status = QStringLiteral("Повреждён");
            info.errorText = QStringLiteral("Не удалось пропустить Global Color Table");
            return info;
        }
    }

    bool gotTrailer = false;
    while (f.pos() < fileSize) {
        const QByteArray typeByte = f.read(1);
        if (typeByte.size() != 1)
            break;
        const quint8 bt = quint8(typeByte[0]);
        if (bt == 0x3B) {
            gotTrailer = true;
            break;
        }
        if (bt == 0x2C) {
            const QByteArray desc = f.read(9);
            if (desc.size() != 9) {
                info.status = QStringLiteral("Повреждён");
                info.errorText = QStringLiteral("Обрыв Image Descriptor");
                return info;
            }
            const quint8 dPacked = quint8(desc[8]);
            if (dPacked & 0x80) {
                const int n = dPacked & 0x07;
                const qint64 lctSz = qint64(3) * (1 << (n + 1));
                const qint64 next = f.pos() + lctSz;
                if (next > fileSize) {
                    info.status = QStringLiteral("Повреждён");
                    info.errorText  = QStringLiteral("Local Color Table выходит за пределы файла");
                    return info;
                }
                if (!f.seek(next)) {
                    info.status = QStringLiteral("Повреждён");
                    info.errorText  = QStringLiteral("Не удалось пропустить Local Color Table");
                    return info;
                }
            }
            const QByteArray lzwMin = f.read(1);
            if (lzwMin.size() != 1) {
                info.status = QStringLiteral("Повреждён");
                info.errorText  = QStringLiteral("Обрыв перед LZW-данными кадра");
                return info;
            }
            if (!gifSkipSubBlocks(f, fileSize)) {
                info.status = QStringLiteral("Повреждён");
                info.errorText  = QStringLiteral("Обрыв в sub-blocks кадра");
                return info;
            }
        }
        else if (bt == 0x21) {
            const QByteArray label = f.read(1);
            if (label.size() != 1) {
                info.status = QStringLiteral("Повреждён");
                info.errorText  = QStringLiteral("Обрыв метки Extension");
                return info;
            }
            if (!gifSkipSubBlocks(f, fileSize)) {
                info.status = QStringLiteral("Повреждён");
                info.errorText = QStringLiteral("Обрыв в sub-blocks расширения");
                return info;
            }
        }
        else {
            info.status = QStringLiteral("Повреждён");
            info.errorText = QStringLiteral("Неизвестный тип блока");
            return info;
        }
    }

    if (!gotTrailer) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Отсутствует Trailer (0x3B) в конце файла");
        return info;
    }

    info.width = width;
    info.height = height;
    info.colorDepth = gifDescribeColorDepth(hasGCT, packed);
    info.compression = QStringLiteral("LZW");
    info.status = QStringLiteral("OK");
    return info;
}



static QString pcxDescribeVersion(quint8 v)
{
    switch (v) {
    case 0: return QStringLiteral("v2.5");
    case 2: return QStringLiteral("v2.8 (с палитрой)");
    case 3: return QStringLiteral("v2.8 (без палитры)");
    case 4: return QStringLiteral("v3.0 (Paintbrush for Windows)");
    case 5: return QStringLiteral("v3.0+");
    default: return QStringLiteral("v%1").arg(v);
    }
}

static QString pcxDescribeColorDepth(quint8 bitsPerPixel, quint8 planes)
{
    const int total = int(bitsPerPixel) * int(planes);
    if (planes == 1) {
        switch (bitsPerPixel) {
        case 1: return QStringLiteral("1 бит (монохром)");
        case 2: return QStringLiteral("2 бита (4 цвета)");
        case 4: return QStringLiteral("4 бита (16 цветов, палитра)");
        case 8: return QStringLiteral("8 бит (256 цветов, палитра)");
        }
    }
    if (bitsPerPixel == 8 && planes == 3)
        return QStringLiteral("24 бита (RGB, 8 бит/канал)");
    if (bitsPerPixel == 8 && planes == 4)
        return QStringLiteral("32 бита (RGBA, 8 бит/канал)");
    return QStringLiteral("%1 бит (%2 бит × %3 плоскости)").arg(total).arg(bitsPerPixel).arg(planes);
}

bool PCXParser::canParse(const QString &filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const QByteArray b = f.read(1);
    return b.size() == 1 && quint8(b[0]) == 0x0A;
}

ImageInfo PCXParser::parse(const QString &filePath) const
{
    ImageInfo info;
    info.fileName = QFileInfo(filePath).fileName();
    info.filePath = filePath;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Не удалось открыть файл");
        return info;
    }

    if (f.size() < 128) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Файл меньше 128 байт (заголовок PCX)");
        return info;
    }

    const QByteArray h = f.read(80);
    if (h.size() != 80) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Обрыв заголовка PCX");
        return info;
    }

    if (quint8(h[0]) != 0x0A) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Неверная сигнатура PCX (ожидалось 0x0A)");
        return info;
    }

    const quint8  version = quint8(h[1]);
    const quint8  encoding = quint8(h[2]);
    const quint8  bitsPerPixel = quint8(h[3]);
    const quint16 xMin = ByteUtils::readU16LE(h, 4);
    const quint16 yMin = ByteUtils::readU16LE(h, 6);
    const quint16 xMax = ByteUtils::readU16LE(h, 8);
    const quint16 yMax = ByteUtils::readU16LE(h, 10);
    const quint16 hDpi = ByteUtils::readU16LE(h, 12);
    const quint16 vDpi = ByteUtils::readU16LE(h, 14);
    const quint8  nPlanes = quint8(h[65]);

    if (encoding > 1) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Неизвестное значение Encoding: %1").arg(encoding);
        return info;
    }
    if (bitsPerPixel != 1 && bitsPerPixel != 2
        && bitsPerPixel != 4 && bitsPerPixel != 8) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Некорректный BitsPerPixel: %1").arg(bitsPerPixel);
        return info;
    }
    if (nPlanes < 1 || nPlanes > 4) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Некорректный NPlanes: %1").arg(nPlanes);
        return info;
    }
    if (xMax < xMin || yMax < yMin) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Некорректные границы изображения");
        return info;
    }

    const int width  = int(xMax) - int(xMin) + 1;
    const int height = int(yMax) - int(yMin) + 1;
    if (width <= 0 || height <= 0) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Нулевые размеры изображения");
        return info;
    }

    info.width = width;
    info.height = height;
    info.colorDepth  = pcxDescribeColorDepth(bitsPerPixel, nPlanes) + QStringLiteral(" (") + pcxDescribeVersion(version) + QStringLiteral(")");
    info.compression = (encoding == 1) ? QStringLiteral("RLE (PCX Run-Length Encoding)") : QStringLiteral("Без сжатия)");
    if (hDpi > 0) info.dpiX = hDpi;
    if (vDpi > 0) info.dpiY = vDpi;

    info.status = QStringLiteral("OK");
    return info;
}




static quint16 tiffReadU16(const QByteArray &b, int off, bool le)
{
    const quint8 b0 = quint8(b[off]);
    const quint8 b1 = quint8(b[off + 1]);
    return le ? (quint16(b0) | (quint16(b1) << 8)) : ((quint16(b0) << 8) | quint16(b1));
}

static quint32 tiffReadU32(const QByteArray &b, int off, bool le)
{
    const quint8 b0 = quint8(b[off]);
    const quint8 b1 = quint8(b[off + 1]);
    const quint8 b2 = quint8(b[off + 2]);
    const quint8 b3 = quint8(b[off + 3]);
    if (le)
        return  quint32(b0) | (quint32(b1) <<  8) | (quint32(b2) << 16) | (quint32(b3) << 24);
    return (quint32(b0) << 24) | (quint32(b1) << 16) | (quint32(b2) <<  8) | quint32(b3);
}

static int tiffTypeSize(quint16 type)
{
    switch (type) {
    case 1:  return 1;  // BYTE
    case 2:  return 1;  // ASCII
    case 3:  return 2;  // SHORT
    case 4:  return 4;  // LONG
    case 5:  return 8;  // RATIONAL
    case 6:  return 1;  // SBYTE
    case 7:  return 1;  // UNDEFINED
    case 8:  return 2;  // SSHORT
    case 9:  return 4;  // SLONG
    case 10: return 8;  // SRATIONAL
    case 11: return 4;  // FLOAT
    case 12: return 8;  // DOUBLE
    default: return 0;
    }
}

static QString tiffDescribeCompression(quint16 code)
{
    switch (code) {
    case 1: return QStringLiteral("Без сжатия");
    case 2: return QStringLiteral("CCITT Group 3 (1D)");
    case 3: return QStringLiteral("CCITT Group 4 (fax)");
    case 4: return QStringLiteral("CCITT Group 3 (T.4)");
    case 5: return QStringLiteral("LZW");
    case 6: return QStringLiteral("JPEG (устаревший)");
    case 7: return QStringLiteral("JPEG");
    case 8: return QStringLiteral("Deflate (Adobe)");
    case 32773: return QStringLiteral("PackBits (RLE)");
    case 32946: return QStringLiteral("Deflate");
    default: return QStringLiteral("Неизвестный код сжатия (%1)").arg(code);
    }
}

static QString tiffDescribePhotometric(quint16 code)
{
    switch (code) {
    case 0: return QStringLiteral("оттенки белого");
    case 1: return QStringLiteral("оттенки чёрного");
    case 2: return QStringLiteral("RGB");
    case 3: return QStringLiteral("палитра");
    case 4: return QStringLiteral("маска (transparency)");
    case 5: return QStringLiteral("CMYK");
    case 6: return QStringLiteral("YCbCr");
    case 8: return QStringLiteral("CIELab");
    default: return QStringLiteral("Photometric=%1").arg(code);
    }
}

static QString tiffDescribeColorDepth(const QVector<quint16> &bitsPerSample, int samplesPerPixel, quint16 photometric)
{
    if (bitsPerSample.isEmpty())
        return QStringLiteral("%1 канал(а), глубина неизвестна").arg(samplesPerPixel);

    int total = 0;
    bool allSame = true;
    const int first = bitsPerSample.first();
    for (int v : bitsPerSample) {
        total += v;
        if (v != first) allSame = false;
    }

    if (allSame) {
        return QStringLiteral("%1 бит (%2 канал(а) × %3 бит, %4)").arg(total).arg(samplesPerPixel).arg(first).arg(tiffDescribePhotometric(photometric));
    }
    return QStringLiteral("%1 бит (несимметричные каналы)").arg(total);
}

bool TIFFParser::canParse(const QString &filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const QByteArray head = f.read(4);
    if (head.size() != 4)
        return false;
    const QByteArray order = head.left(2);
    if (order != QByteArrayLiteral("II") && order != QByteArrayLiteral("MM"))
        return false;
    const bool le = (order == QByteArrayLiteral("II"));
    return tiffReadU16(head, 2, le) == 42;
}

ImageInfo TIFFParser::parse(const QString &filePath) const
{
    ImageInfo info;
    info.fileName = QFileInfo(filePath).fileName();
    info.filePath = filePath;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Не удалось открыть файл");
        return info;
    }

    const qint64 fileSize = f.size();
    if (fileSize < 8) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Файл меньше 8 байт (заголовок TIFF)");
        return info;
    }

    const QByteArray header = f.read(8);
    if (header.size() != 8) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Обрыв заголовка TIFF");
        return info;
    }

    const QByteArray order = header.left(2);
    bool littleEndian;
    if (order == QByteArrayLiteral("II")) {
        littleEndian = true;
    } else if (order == QByteArrayLiteral("MM")) {
        littleEndian = false;
    } else {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Неверный порядок байт TIFF");
        return info;
    }

    if (tiffReadU16(header, 2, littleEndian) != 42) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Неверный magic number TIFF (ожидалось 42)");
        return info;
    }

    const quint32 firstIFDOffset = tiffReadU32(header, 4, littleEndian);
    if (firstIFDOffset == 0 || qint64(firstIFDOffset) + 2 > fileSize) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Некорректное смещение первого IFD");
        return info;
    }

    if (!f.seek(firstIFDOffset)) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Не удалось перейти к IFD");
        return info;
    }
    const QByteArray countBytes = f.read(2);
    if (countBytes.size() != 2) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Обрыв при чтении числа тегов IFD");
        return info;
    }
    const quint16 tagCount = tiffReadU16(countBytes, 0, littleEndian);

    const qint64 ifdBytes = qint64(tagCount) * 12;
    if (f.pos() + ifdBytes > fileSize) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("IFD выходит за пределы файла");
        return info;
    }

    const QByteArray ifd = f.read(int(ifdBytes));
    if (ifd.size() != int(ifdBytes)) {
        info.status = QStringLiteral("Повреждён");
        info.errorText = QStringLiteral("Обрыв данных IFD");
        return info;
    }

    int width = 0;
    int height = 0;
    int samplesPerPixel = 0;
    quint16 compression = 0;
    quint16 photometric = 0;
    quint16 resolutionUnit = 0;
    double xRes = 0.0;
    double yRes = 0.0;
    QVector<quint16> bitsPerSample;

    for (int i = 0; i < tagCount; ++i) {
        const int off = i * 12;
        const quint16 tag = tiffReadU16(ifd, off,     littleEndian);
        const quint16 type = tiffReadU16(ifd, off + 2, littleEndian);
        const quint32 count = tiffReadU32(ifd, off + 4, littleEndian);
        const int tsize = tiffTypeSize(type);
        if (tsize == 0)
            continue;
        const qint64 total = qint64(count) * tsize;

        QByteArray value;
        if (total <= 4) {
            value = ifd.mid(off + 8, int(total));
        } else {
            const quint32 valueOffset = tiffReadU32(ifd, off + 8, littleEndian);
            if (qint64(valueOffset) + total > fileSize)
                continue;
            const qint64 saved = f.pos();
            if (!f.seek(valueOffset))
                continue;
            value = f.read(int(total));
            f.seek(saved);
            if (value.size() != int(total))
                continue;
        }

        switch (tag) {
        case 256:  // ImageWidth
            if (tsize == 2 && count >= 1) width = tiffReadU16(value, 0, littleEndian);
            else if (tsize == 4 && count >= 1) width = int(tiffReadU32(value, 0, littleEndian));
            break;
        case 257:  // ImageLength
            if (tsize == 2 && count >= 1) height = tiffReadU16(value, 0, littleEndian);
            else if (tsize == 4 && count >= 1) height = int(tiffReadU32(value, 0, littleEndian));
            break;
        case 258:  // BitsPerSample
            if (tsize == 2) {
                for (quint32 k = 0; k < count; ++k)
                    bitsPerSample.append(tiffReadU16(value, int(k) * 2, littleEndian));
            }
            break;
        case 259:  // Compression
            if (tsize == 2 && count >= 1) compression = tiffReadU16(value, 0, littleEndian);
            break;
        case 262:  // PhotometricInterpretation
            if (tsize == 2 && count >= 1) photometric = tiffReadU16(value, 0, littleEndian);
            break;
        case 277:  // SamplesPerPixel
            if (tsize == 2 && count >= 1) samplesPerPixel = tiffReadU16(value, 0, littleEndian);
            break;
        case 282:  // XResolution
            if (tsize == 8 && count >= 1) {
                const quint32 num = tiffReadU32(value, 0, littleEndian);
                const quint32 den = tiffReadU32(value, 4, littleEndian);
                if (den != 0) xRes = double(num) / double(den);
            }
            break;
        case 283:  // YResolution
            if (tsize == 8 && count >= 1) {
                const quint32 num = tiffReadU32(value, 0, littleEndian);
                const quint32 den = tiffReadU32(value, 4, littleEndian);
                if (den != 0) yRes = double(num) / double(den);
            }
            break;
        case 296:  // ResolutionUnit
            if (tsize == 2 && count >= 1) resolutionUnit = tiffReadU16(value, 0, littleEndian);
            break;
        default:
            break;
        }
    }

    if (width <= 0 || height <= 0) {
        info.status = QStringLiteral("Повреждён");
        info.errorText  = QStringLiteral("Отсутствуют теги ImageWidth/ImageLength");
        return info;
    }

    if (samplesPerPixel <= 0)
        samplesPerPixel = bitsPerSample.size() > 0 ? bitsPerSample.size() : 1;

    info.width = width;
    info.height = height;
    info.colorDepth  = tiffDescribeColorDepth(bitsPerSample, samplesPerPixel, photometric);
    info.compression = compression != 0 ? tiffDescribeCompression(compression) : QStringLiteral("Без сжатия");

    if (resolutionUnit == 2) {
        if (xRes > 0) info.dpiX = xRes;
        if (yRes > 0) info.dpiY = yRes;
    } else if (resolutionUnit == 3) {
        if (xRes > 0) info.dpiX = xRes * 2.54;
        if (yRes > 0) info.dpiY = yRes * 2.54;
    }

    info.status = QStringLiteral("OK");
    return info;
}



static const QSet<QString> &parserExtSet()
{
    static const QSet<QString> s = {
        QStringLiteral("jpg"),
        QStringLiteral("jpeg"),
        QStringLiteral("gif"),
        QStringLiteral("tif"),
        QStringLiteral("tiff"),
        QStringLiteral("bmp"),
        QStringLiteral("png"),
        QStringLiteral("pcx")
    };
    return s;
}

static const PNGParser p_png;
static const BMPParser p_bmp;
static const JPEGParser p_jpg;
static const GIFParser p_gif;
static const PCXParser p_pcx;
static const TIFFParser p_tif;

bool ParserRegistry::isSupportedExtension(const QString &filePath)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    return !ext.isEmpty() && parserExtSet().contains(ext);
}

ImageInfo ParserRegistry::parseFile(const QString &filePath)
{
    if (p_png.canParse(filePath)) return p_png.parse(filePath);
    if (p_bmp.canParse(filePath)) return p_bmp.parse(filePath);
    if (p_jpg.canParse(filePath)) return p_jpg.parse(filePath);
    if (p_gif.canParse(filePath)) return p_gif.parse(filePath);
    if (p_pcx.canParse(filePath)) return p_pcx.parse(filePath);
    if (p_tif.canParse(filePath)) return p_tif.parse(filePath);

    ImageInfo info;
    info.fileName = QFileInfo(filePath).fileName();
    info.filePath = filePath;
    info.status = QStringLiteral("Не поддерживается");
    return info;
}