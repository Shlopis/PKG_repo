#include "mainwindow.h"
#include "cohensutherland.h"
#include "weileratherton.h"
#include "stresstest.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QLocale>
#include <QFile>
#include <algorithm>

static QVector<PolygonContour> presetVipukliy() {
    return { PolygonContour{ { {-30,-20}, {10,-30}, {35,0}, {15,30}, {-20,25} }, false } };
}
static QVector<PolygonContour> presetVognutiy() {
    return { PolygonContour{ { {-35,-25}, {30,-25}, {30,10}, {5,10}, {5,30}, {-35,30} }, false } };
}
static QVector<PolygonContour> presetSamoperesec() {
    return { PolygonContour{ { {-10,-10}, {40,40}, {-40,40}, {10,-10} }, false } };
}
static QVector<PolygonContour> presetWithHole() {
    QVector<PolygonContour> result;
    result.push_back(PolygonContour{ { {-35,-35}, {35,-35}, {35,35}, {-35,35} }, false });
    result.push_back(PolygonContour{ { {-12,-12}, {12,-12}, {12,12}, {-12,12} }, true  });
    return result;
}

static QVector<Point> clipper1() {
    return { {-50,-30}, {0,-10}, {50,-30}, {50,30}, {0,10}, {-50,30} };
}
static QVector<Point> clipperL() {
    return { {-45,-35}, {25,-40}, {20,-5}, {-5,0}, {-10,35}, {-50,30} };
}


static QVector<Point> presetPolygonClipper() {
    //return clipper1();
    return clipperL();
}

static void ensureCounterClockwise(QVector<Point>& polygon) {
    if (WeilerAtherton::polygonArea(polygon) < 0.0)
        std::reverse(polygon.begin(), polygon.end());
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Clip");
    resize(900, 750);

    scene = new ScenePainter(this);

    auto* loadButton = new QPushButton("Загрузить файл", this);
    auto* fitButton = new QPushButton("Центр", this);
    auto* stressButton = new QPushButton("Стресс-тест", this);

    originalCheckbox = new QCheckBox("Исходные отрезки", this);
    clippedCheckbox = new QCheckBox("Видимые части", this);
    quadTreeCheckbox = new QCheckBox("QuadTree", this);
    originalCheckbox->setChecked(true);
    clippedCheckbox->setChecked(true);

    polygonCombo = new QComboBox(this);
    polygonCombo->addItem("Полигон: нет");
    polygonCombo->addItem("Полигон: выпуклый");
    polygonCombo->addItem("Полигон: вогнутый");
    polygonCombo->addItem("Полигон: самопересекающийся");
    polygonCombo->addItem("Полигон: с дырой");

    polygonClipper = presetPolygonClipper();
    ensureCounterClockwise(polygonClipper);
    scene->setClipPolygon(polygonClipper, true);
    scene->setClipRect(currentWindow(), true);

    connect(loadButton, &QPushButton::clicked, this, &MainWindow::onLoad);
    connect(fitButton, &QPushButton::clicked, this, &MainWindow::onFit);
    connect(stressButton, &QPushButton::clicked, this, &MainWindow::onStress);
    connect(polygonCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onPolygonChanged);

    auto refreshFlags = [this]() {
        scene->setFlags(originalCheckbox->isChecked(), clippedCheckbox->isChecked(), quadTreeCheckbox->isChecked(), true);
    };
    connect(originalCheckbox, &QCheckBox::toggled, this, refreshFlags);
    connect(clippedCheckbox, &QCheckBox::toggled, this, refreshFlags);
    connect(quadTreeCheckbox, &QCheckBox::toggled, this, refreshFlags);

    connect(scene, &ScenePainter::clipRectChanged, this, [this](const Rect& rect) {
        clipRect = rect;
        hasClipRect = true;
        runClipping();
    });
    connect(scene, &ScenePainter::clipPolygonChanged, this, [this](const QVector<Point>& polygon) {
        polygonClipper = polygon;
        updatePolygons();
    });

    auto* topRow = new QHBoxLayout;
    topRow->addWidget(loadButton);
    topRow->addWidget(fitButton);
    topRow->addWidget(originalCheckbox);
    topRow->addWidget(clippedCheckbox);
    topRow->addWidget(quadTreeCheckbox);
    topRow->addStretch(1);

    auto* polygonRow = new QHBoxLayout;
    polygonRow->addWidget(polygonCombo);
    polygonRow->addStretch(1);
    polygonRow->addWidget(stressButton);

    auto* rootLayout = new QVBoxLayout;
    rootLayout->addLayout(topRow);
    rootLayout->addLayout(polygonRow);
    rootLayout->addWidget(scene, 1);

    auto* centralWidget = new QWidget(this);
    centralWidget->setLayout(rootLayout);
    setCentralWidget(centralWidget);

    refreshFlags();
}

void MainWindow::onFit() { scene->fitToContent(); }

Rect MainWindow::currentWindow() const {
    if (hasClipRect) return clipRect;
    return Rect{ -25, -20, 25, 20 };
}

void MainWindow::onLoad() {
    const QString path = QFileDialog::getOpenFileName(this, "Входные данные", QString(), "Data (*.txt)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл");
        return;
    }
    QTextStream input(&file);
    input.setLocale(QLocale::C);

    segments.clear();
    clippedSegments.clear();
    hasClipRect = false;

    int count = 0;
    input >> count;
    if (count < 0 || (count > 5000000)) {
        QMessageBox::critical(this, "Ошибка", "Некорректное n");
        return;
    }
    for (int i = 0; i < count; ++i) {
        Segment segment;
        input >> segment.p1.x >> segment.p1.y >> segment.p2.x >> segment.p2.y;
        if (input.status() != QTextStream::Ok) {
            QMessageBox::critical(this, "Ошибка", QString("Обрыв данных на отрезке %1").arg(i + 1));
            return;
        }
        segments.push_back(segment);
    }
    input >> clipRect.xmin >> clipRect.ymin >> clipRect.xmax >> clipRect.ymax;
    if (input.status() != QTextStream::Ok) {
        QMessageBox::critical(this, "Ошибка", "Нет строки с координатами окна");
        return;
    }
    hasClipRect = true;
    file.close();

    Rect domain = clipRect;
    for (const Segment& segment : segments) {
        const Rect box = segAABB(segment);
        domain.xmin = std::min(domain.xmin, box.xmin);
        domain.ymin = std::min(domain.ymin, box.ymin);
        domain.xmax = std::max(domain.xmax, box.xmax);
        domain.ymax = std::max(domain.ymax, box.ymax);
    }
    const double padding = std::max(1.0, (domain.xmax - domain.xmin) * 0.05);
    domain.xmin -= padding; domain.ymin -= padding;
    domain.xmax += padding; domain.ymax += padding;
    quadTree.build(segments, domain);

    scene->setClipRect(clipRect, true);
    scene->setQuadTreeRects(quadTree.nodeRects());
    scene->fitToContent();

    runClipping();
    updatePolygons();
}

void MainWindow::runClipping() {
    clippedSegments.clear();
    if (hasClipRect && !clipRect.degenerate()) {
        const QVector<int> candidates = quadTree.query(clipRect);
        for (int id : candidates) {
            const ClipResult result = clipSegmentCS(segments[id], clipRect);
            if (result.visible)
                clippedSegments.push_back(result.segment);
        }
    }
    scene->setSegments(segments, clippedSegments);
    scene->setQuadTreeRects(quadTree.nodeRects());
}

void MainWindow::onPolygonChanged() {
    switch (polygonCombo->currentIndex()) {
    case 1: polygons = presetVipukliy(); break;
    case 2: polygons = presetVognutiy(); break;
    case 3: polygons = presetSamoperesec(); break;
    case 4: polygons = presetWithHole(); break;
    default: polygons.clear(); break;
    }
    updatePolygons();
}

void MainWindow::updatePolygons() {
    clippedPolygons.clear();
    if (!polygons.isEmpty() && polygonClipper.size() >= 3)
        clippedPolygons = WeilerAtherton::clip(polygons, polygonClipper);
    scene->setPolygons(polygons, clippedPolygons);
}

void MainWindow::onStress() {
    const StressReport report = runStress(currentWindow());
    const QString text =
        QString("%1 отрезков\n QuadTree: узлов %2, построение %3 мс\n Через quadtree: кандидатов %4, видимых %5, время %6 мс\n Полный перебор: видимых %7, время %8 мс\n"
                "Ускорение отсечения: ×%9")
            .arg(report.total)
            .arg(report.nodes)
            .arg(report.buildMs, 0, 'f', 1)
            .arg(report.candidates)
            .arg(report.visibleQuad)
            .arg(report.qtClipMs, 0, 'f', 1)
            .arg(report.visibleBrute)
            .arg(report.bruteMs, 0, 'f', 1)
            .arg(report.qtClipMs > 0 ? report.bruteMs / report.qtClipMs : 0.0, 0, 'f', 2);
    QMessageBox::information(this, "Стресс-тест", text);
}