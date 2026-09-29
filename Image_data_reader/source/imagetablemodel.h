#ifndef IMAGETABLEMODEL_H
#define IMAGETABLEMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include "imageinfo.h"

class ImageTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColFileName = 0,
        ColSize,
        ColDpi,
        ColColorDepth,
        ColCompression,
        ColStatus,
        ColumnCount
    };

    explicit ImageTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orient, int role) const override;

    void clear();
    void appendBatch(const QVector<ImageInfo> &batch);

private:
    QVector<ImageInfo> tablerows;
};
#endif // IMAGETABLEMODEL_H
