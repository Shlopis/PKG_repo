 #include "benchmarkwidget.h"
#include <QPainter>
#include <algorithm>

BenchmarkWidget::BenchmarkWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(600, 450);
}

void BenchmarkWidget::setResults(const QVector<BenchmarkResult>& results) {
    m_results = results;
    update();
}

void BenchmarkWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (m_results.isEmpty()) {
        p.drawText(rect(), Qt::AlignCenter, "Нет данных");
        return;
    }

    const int left = 70, top = 45, right = 30, bottom = 65;
    QRect chart(left, top, width() - left - right, height() - top - bottom);

    double maxMs = 0;
    for (const auto& r : m_results)
        maxMs = std::max(maxMs, r.timeMs);
    if (maxMs <= 0)
        maxMs = 1;

    p.setPen(QPen(Qt::black, 2));
    p.drawLine(chart.left(), chart.top(), chart.left(), chart.bottom());
    p.drawLine(chart.left(), chart.bottom(), chart.right(), chart.bottom());

    QFont small = p.font(); small.setPointSize(8); p.setFont(small);
    for (int i = 1; i <= 4; ++i) {
        int y = chart.bottom() - int(chart.height() * i / 4.0);
        p.setPen(QPen(QColor(210, 210, 210), 1));
        p.drawLine(chart.left(), y, chart.right(), y);
        p.setPen(Qt::black);
        p.drawText(QRect(0, y - 8, left - 8, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(maxMs * i / 4.0, 'f', 1));
    }

    const int n = m_results.size();
    const double slot = chart.width() / double(n);
    const int barW = int(slot * 0.5);

    for (int i = 0; i < n; ++i) {
        const auto& r = m_results[i];
        int barH = int(chart.height() * (r.timeMs / maxMs));
        if (barH < 1 && r.timeMs > 0)
            barH = 1;
        int x = chart.left() + int(slot * i + (slot - barW) / 2);
        int y = chart.bottom() - barH;

        p.fillRect(x, y, barW, barH, QColor::fromHsv((i * 360) / n, 160, 220));
        p.setPen(QPen(Qt::black, 1));
        p.drawRect(x, y, barW, barH);

        p.setPen(Qt::black);
        p.drawText(QRect(chart.left() + int(slot * i), y - 20, int(slot), 18), Qt::AlignHCenter | Qt::AlignVCenter, QString::number(r.timeMs, 'f', 2) + " ms");

        p.drawText(QRect(chart.left() + int(slot * i), chart.bottom() + 6, int(slot), 44), Qt::AlignHCenter | Qt::AlignTop, r.name + "\n(" + r.primitive + ")");
    }

    QFont title = p.font(); title.setPointSize(11); title.setBold(true);
    p.setFont(title);
    p.drawText(QRect(0, 8, width(), 26), Qt::AlignHCenter, "Время работы на стресс-тестировании");
}