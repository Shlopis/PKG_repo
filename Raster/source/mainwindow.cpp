#include "mainwindow.h"
#include "rasterengine.h"
#include "benchmark.h"
#include "benchmarkwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QScrollArea>
#include <QRandomGenerator>
#include <QDialog>
#include <QApplication>
#include <cmath>

static int radiusFromOrigin(int x, int y) {
    return int(std::round(std::hypot(double(x), double(y))));
}                         //кв. корень суммы квадратов

static bool isCircleAlgo(int index) { return index == 3 || index == 6; }

static void drawAlgorithm(int index, int x0, int y0, int x1, int y1, IRasterTarget& t) {
    switch (index) {
    case 0: RasterEngine::lineStep(x0, y0, x1, y1, t); break;
    case 1: RasterEngine::lineDDA(x0, y0, x1, y1, t); break;
    case 2: RasterEngine::lineBresenham(x0, y0, x1, y1, t); break;
    case 3: RasterEngine::circleBresenham(radiusFromOrigin(x1, y1), t); break;
    case 4: RasterEngine::lineCastlePitwey(x0, y0, x1, y1, t); break;
    case 5: RasterEngine::lineWu(x0, y0, x1, y1, t); break;
    case 6: RasterEngine::circleWu(radiusFromOrigin(x1, y1), t); break;
    }
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), gridA(-30, -30, 61, 61), gridB(-30, -30, 61, 61)
{
    setWindowTitle("Raster");
    resize(950, 680);

    painterA = new GridPainter(this);
    painterB = new GridPainter(this);
    painterA->setGrid(&gridA);
    painterB->setGrid(&gridB);
    painterA->setCompareGrid(&gridB);
    painterB->setCompareGrid(&gridA);

    QScrollArea* scrollA = new QScrollArea(this);
    QScrollArea* scrollB = new QScrollArea(this);
    scrollA->setWidget(painterA);
    scrollB->setWidget(painterB);
    scrollA->setWidgetResizable(true);
    scrollB->setWidgetResizable(true);

    QSplitter* splitter = new QSplitter(this);
    splitter->addWidget(scrollA);
    splitter->addWidget(scrollB);

    comboA = new QComboBox(this);
    comboB = new QComboBox(this);
    for (QComboBox* c : {comboA, comboB}) {
        c->addItem("1. Пошаговый");
        c->addItem("2. ЦДА (DDA)");
        c->addItem("3. Брезенхем");
        c->addItem("4. Окружность: Брезенхем");
        c->addItem("5. Кастл–Питвей");
        c->addItem("6. Ву (линия)");
        c->addItem("7. Ву (окружность)");
    }
    comboA->setCurrentIndex(2);
    comboB->setCurrentIndex(0);

    auto makeSpin = [this](int v) {
        QSpinBox* s = new QSpinBox(this);
        s->setRange(gridA.minX(), gridA.maxX());
        s->setValue(v);
        return s;
    };
    s_x0 = makeSpin(-20);
    s_y0 = makeSpin(-10);
    s_x1 = makeSpin(20);
    s_y1 = makeSpin(15);

    diffLabel = new QLabel("Расхождений: —", this);
    diffLabel->setToolTip("B — точка, задающая радиус, А не используется");

    QPushButton* btnDraw = new QPushButton("Построить", this);
    QPushButton* btnRandom = new QPushButton("Случайная линия", this);
    QPushButton* btnClear = new QPushButton("Очистить", this);
    QPushButton* btnBench = new QPushButton("Стресс-тест", this);
    QPushButton* btnZoomIn = new QPushButton("+", this);
    QPushButton* btnZoomOut = new QPushButton("−", this);

    connect(btnDraw, &QPushButton::clicked, this, &MainWindow::onDraw);
    connect(btnRandom, &QPushButton::clicked, this, &MainWindow::onRandom);
    connect(btnClear, &QPushButton::clicked, this, &MainWindow::onClear);
    connect(btnBench, &QPushButton::clicked, this, &MainWindow::onBenchmark);

    connect(btnZoomIn, &QPushButton::clicked, this, [this]() {
        painterA->zoomIn();  painterB->zoomIn();
    });
    connect(btnZoomOut, &QPushButton::clicked, this, [this]() {
        painterA->zoomOut(); painterB->zoomOut();
    });

    QHBoxLayout* headA = new QHBoxLayout;
    headA->addWidget(new QLabel("Панель A:", this));
    headA->addWidget(comboA);
    QHBoxLayout* headB = new QHBoxLayout;
    headB->addWidget(new QLabel("Панель B:", this));
    headB->addWidget(comboB);
    headB->addWidget(diffLabel);

    QHBoxLayout* top = new QHBoxLayout;
    top->addLayout(headA);
    top->addLayout(headB);
    top->addWidget(btnZoomOut);
    top->addWidget(btnZoomIn);

    QHBoxLayout* coords = new QHBoxLayout;
    coords->addWidget(new QLabel("A(", this));
    coords->addWidget(s_x0);
    coords->addWidget(new QLabel(",", this));
    coords->addWidget(s_y0);
    coords->addWidget(new QLabel(")  B(", this));
    coords->addWidget(s_x1);
    coords->addWidget(new QLabel(",", this));
    coords->addWidget(s_y1);
    coords->addWidget(new QLabel(")", this));
    coords->setAlignment(Qt::AlignCenter);

    QHBoxLayout* btns = new QHBoxLayout;
    btns->addWidget(btnDraw); btns->addWidget(btnRandom);
    btns->addWidget(btnClear); btns->addWidget(btnBench);

    QVBoxLayout* root = new QVBoxLayout;
    root->addLayout(top);
    root->addWidget(splitter, 1);
    root->addLayout(coords);
    root->addLayout(btns);

    QWidget* central = new QWidget(this);
    central->setLayout(root);
    setCentralWidget(central);
}

void MainWindow::onDraw() {
    int x0 = s_x0->value(), y0 = s_y0->value();
    int x1 = s_x1->value(), y1 = s_y1->value();

    gridA.clear();
    gridB.clear();
    drawAlgorithm(comboA->currentIndex(), x0, y0, x1, y1, gridA);
    drawAlgorithm(comboB->currentIndex(), x0, y0, x1, y1, gridB);

    int diff = 0;
    for (int y = gridA.minY(); y <= gridA.maxY(); ++y)
        for (int x = gridA.minX(); x <= gridA.maxX(); ++x)
            if ((gridA.at(x, y) > 0) != (gridB.at(x, y) > 0))
                ++diff;

    diffLabel->setText(QString("Расхождений: %1").arg(diff));
    painterA->update();
    painterB->update();
}

void MainWindow::onRandom() {
    auto* rng = QRandomGenerator::global();
    s_x0->setValue(gridA.minX() + rng->bounded(gridA.width()));
    s_y0->setValue(gridA.minY() + rng->bounded(gridA.height()));
    s_x1->setValue(gridA.minX() + rng->bounded(gridA.width()));
    s_y1->setValue(gridA.minY() + rng->bounded(gridA.height()));
    onDraw();
}

void MainWindow::onClear() {
    gridA.clear();
    gridB.clear();
    painterA->update();
    painterB->update();
}

void MainWindow::onBenchmark() {
    const int idxA = comboA->currentIndex();
    const int idxB = comboB->currentIndex();

    QVector<int> algos = {idxA};
    if (idxB != idxA)
        algos.append(idxB);

    QVector<BenchmarkResult> results;
    for (int idx : algos) {
        if (isCircleAlgo(idx))
            results.append(Benchmark::runCircle(idx, 50000, gridA));
        else
            results.append(Benchmark::runLine(idx, 100000, gridA));
    }

    QDialog* dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle("Стресс-тест алгоритмов");
    dialog->resize(700, 460);

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QLabel* info = new QLabel(QString("Отрезков: 100 000 (веер, 8 октантов) | ""Окружностей: 50 000 | Растр: %1×%2").arg(gridA.width()).arg(gridA.height()), dialog);
    info->setAlignment(Qt::AlignCenter);
    info->setWordWrap(true);
    layout->addWidget(info);

    BenchmarkWidget* bench = new BenchmarkWidget(dialog);
    bench->setResults(results);
    layout->addWidget(bench);

    dialog->show();
}