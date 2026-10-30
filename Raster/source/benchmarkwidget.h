#ifndef BENCHMARKWIDGET_H
#define BENCHMARKWIDGET_H

#include <QWidget>
#include "benchmark.h"

class BenchmarkWidget : public QWidget {
    Q_OBJECT
public:
    explicit BenchmarkWidget(QWidget* parent = nullptr);
    void setResults(const QVector<BenchmarkResult>& results);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<BenchmarkResult> m_results;
};

#endif // BENCHMARKWIDGET_H
