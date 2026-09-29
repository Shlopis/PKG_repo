#include "imagetablemodel.h"

ImageTableModel::ImageTableModel(QObject *parent) : QAbstractTableModel(parent) {}

int ImageTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : tablerows.size();
}

int ImageTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ImageTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= tablerows.size())
        return {};

    const ImageInfo &row = tablerows.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColFileName: return row.fileName;
        case ColSize: return QStringLiteral("%1 x %2").arg(row.width).arg(row.height);
        case ColDpi: return (row.dpiX > 0.0) ? QString::number(row.dpiX, 'f', 0) : QStringLiteral("-");
        case ColColorDepth: return row.colorDepth.isEmpty() ? QStringLiteral("-") : row.colorDepth;
        case ColCompression: return row.compression.isEmpty() ? QStringLiteral("-") : row.compression;
        case ColStatus: return row.status;
        }
    }

    if (role == Qt::ToolTipRole) {
        if (!row.errorText.isEmpty())
            return row.filePath + '\n' + row.errorText;
        return row.filePath;
    }

    return {};
}

QVariant ImageTableModel::headerData(int section, Qt::Orientation orient, int role) const
{
    if (role != Qt::DisplayRole || orient != Qt::Horizontal)
        return {};

    switch (section) {
    case ColFileName: return QStringLiteral("Имя файла");
    case ColSize: return QStringLiteral("Размер (px)");
    case ColDpi: return QStringLiteral("Разрешение (dpi)");
    case ColColorDepth: return QStringLiteral("Глубина цвета");
    case ColCompression: return QStringLiteral("Сжатие");
    case ColStatus: return QStringLiteral("Статус");
    }
    return {};
}

void ImageTableModel::clear()
{
    beginResetModel();
    tablerows.clear();
    endResetModel();
}

void ImageTableModel::appendBatch(const QVector<ImageInfo> &batch)
{
    if (batch.isEmpty())
        return;
    const int first = tablerows.size();
    const int last  = first + batch.size() - 1;
    beginInsertRows({}, first, last);
    tablerows += batch;
    endInsertRows();
}

