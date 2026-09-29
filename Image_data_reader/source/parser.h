#ifndef PARSER_H
#define PARSER_H

#include <QString>
#include "imageinfo.h"

class ParserGeneral {
public:
    virtual ~ParserGeneral() = default;

    virtual bool canParse(const QString &filePath) const = 0;
    virtual ImageInfo parse(const QString &filePath) const = 0;
};
#endif // PARSER_H
