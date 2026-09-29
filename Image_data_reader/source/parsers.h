#ifndef PARSERS_H
#define PARSERS_H

#include <QString>
#include <QByteArray>
#include <QVector>
#include <QFile>

#include "parser.h"
#include "imageinfo.h"


class ByteUtils {
public:
    static quint16 readU16LE(const QByteArray &b, int off = 0);
    static quint16 readU16BE(const QByteArray &b, int off = 0);
    static quint32 readU32LE(const QByteArray &b, int off = 0);
    static quint32 readU32BE(const QByteArray &b, int off = 0);
    static qint32  readS32LE(const QByteArray &b, int off = 0);
};

class PNGParser : public ParserGeneral {
public:
    bool canParse(const QString &filePath) const override;
    ImageInfo parse(const QString &filePath) const override;

private:
    static int samplesForColorType(int colorType);
    static QString describeColorDepth(int bitDepth, int colorType);
};

class BMPParser : public ParserGeneral {
public:
    bool canParse(const QString &filePath) const override;
    ImageInfo parse(const QString &filePath) const override;

private:
    static QString describeCompression(quint32 code);
    static QString describeColorDepth(quint16 bitCount, quint32 compression);
};

class JPEGParser : public ParserGeneral {
public:
    bool canParse(const QString &filePath) const override;
    ImageInfo parse(const QString &filePath) const override;

private:
    static bool isSOFMarker(quint8 marker);
    static QString describeSOF(quint8 marker);
    static QString describeColorDepth(int precision, int components);
};

class GIFParser : public ParserGeneral {
public:
    bool canParse(const QString &filePath) const override;
    ImageInfo parse(const QString &filePath) const override;

private:
    static bool skipSubBlocks(QFile &f, qint64 fileSize);
    static QString describeColorDepth(bool hasGCT, int packedByte);
};

class PCXParser : public ParserGeneral {
public:
    bool canParse(const QString &filePath) const override;
    ImageInfo parse(const QString &filePath) const override;

private:
    static QString describeVersion(quint8 version);
    static QString describeColorDepth(quint8 bitsPerPixel, quint8 planes);
};

class TIFFParser : public ParserGeneral {
public:
    bool canParse(const QString &filePath) const override;
    ImageInfo parse(const QString &filePath) const override;

private:
    static quint16 readU16(const QByteArray &b, int off, bool littleEndian);
    static quint32 readU32(const QByteArray &b, int off, bool littleEndian);
    static int typeSize(quint16 type);

    static QString describeCompression(quint16 code);
    static QString describePhotometric(quint16 code);
    static QString describeColorDepth(const QVector<quint16> &bitsPerSample, int samplesPerPixel, quint16 photometric);
};

class ParserRegistry {
public:
    static ImageInfo parseFile(const QString &filePath);
    static bool isSupportedExtension(const QString &filePath);
};

#endif // PARSERS_H
