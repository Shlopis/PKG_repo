#ifndef HISTOGRAMWIDGET_H
#define HISTOGRAMWIDGET_H

#include <QWidget>
#include <QPainter>
#include <array>
#include "imageprocessor.h"

class HistogramWidget : public QWidget {
    Q_OBJECT
public:
    explicit HistogramWidget(QWidget *parent = nullptr);
    void setHistogramData(const HistogramData& data, bool isGrayscale);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    HistogramData i_data;
    bool i_isGrayscale;
    bool i_hasData;

    void drawChannel(QPainter& painter, const QRect& rect, const std::array<int, 256>& channelData, const QColor& color);
};

#endif // HISTOGRAMWIDGET_H
