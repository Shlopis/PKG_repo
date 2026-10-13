#include "histogramwidget.h"
#include <algorithm>

HistogramWidget::HistogramWidget(QWidget *parent)
    : QWidget(parent), i_isGrayscale(false), i_hasData(false)
{
    setMinimumSize(256, 150);
    setMaximumHeight(300);
}

void HistogramWidget::setHistogramData(const HistogramData& data, bool isGrayscale) {
    i_data = data;
    i_isGrayscale = isGrayscale;
    i_hasData = true;
    update();
}

void HistogramWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), Qt::black);

    if (!i_hasData) return;

    if (i_isGrayscale) {
        drawChannel(painter, rect(), i_data.red, Qt::gray);
    } else {
        painter.setCompositionMode(QPainter::CompositionMode_Plus);
        drawChannel(painter, rect(), i_data.red,   QColor(255, 0, 0, 180));
        drawChannel(painter, rect(), i_data.green, QColor(0, 255, 0, 180));
        drawChannel(painter, rect(), i_data.blue,  QColor(0, 0, 255, 180));
    }
}

void HistogramWidget::drawChannel(QPainter& painter, const QRect& rect, const std::array<int, 256>& channelData, const QColor& color)
{
    int maxVal = *std::max_element(channelData.begin(), channelData.end());
    if (maxVal == 0) return;

    painter.setPen(Qt::NoPen);
    painter.setBrush(color);

    double xStep = rect.width() / 256.0;
    double yScale = static_cast<double>(rect.height()) / maxVal;

    for (int i = 0; i < 256; ++i) {
        double h = channelData[i] * yScale;
        if (h < 0.5) continue;
        double x = rect.x() + i * xStep;
        painter.drawRect(QRectF(x, rect.bottom() - h, xStep, h));
    }
}
