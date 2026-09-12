#include "mainwindow.h"
#include "colorcontroller.h"
#include <QMessageBox>
#include <QColorDialog>

MainWindow::MainWindow(QWidget* parent): QMainWindow(parent), updating(false) {
    setupUI();
    setupConnections();
    controller = new ColorController(this);
    controller->updateFromRGB(0, 0, 0);
    setFixedSize(800,600);
    setWindowTitle("Цветовой конвертер");
}

MainWindow::~MainWindow() {
    delete controller;
}

void MainWindow::setupUI() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    QHBoxLayout* topPanel = new QHBoxLayout();
    topPanel->addWidget(new QLabel("Освещение:", this));
    StandartBox = new QComboBox(this);
    StandartBox->addItem("D65");
    StandartBox->addItem("D50");
    StandartBox->addItem("E");
    StandartBox->setFixedSize(50,20);
    topPanel->addWidget(StandartBox);
    topPanel->addStretch();
    mainLayout->addLayout(topPanel);

    colorChoose = new QPushButton("Выбор цвета",this);
    colorChoose->setFixedSize(150, 30);
    topPanel->addWidget(colorChoose);

    tests = new QPushButton("Провести тесты цвета", this);
    tests->setFixedSize(200,30);
    topPanel->addWidget(tests);

    QHBoxLayout* columnsLayout = new QHBoxLayout();

    QGroupBox* rgbGroup = new QGroupBox("RGB", this);          //rgb
    QVBoxLayout* rgbLayout = new QVBoxLayout(rgbGroup);

    QHBoxLayout* rLayout = new QHBoxLayout();
    rLayout->addWidget(new QLabel("R:"));
    rgbR = new QLineEdit("255", this);
    rgbR->setFixedWidth(50);
    rLayout->addWidget(rgbR);
    rSlider = new QSlider(Qt::Horizontal, this);
    rSlider->setRange(0, 255);
    rLayout->addWidget(rSlider);
    rgbLayout->addLayout(rLayout);

    QHBoxLayout* gLayout = new QHBoxLayout();
    gLayout->addWidget(new QLabel("G:"));
    rgbG = new QLineEdit("0", this);
    rgbG->setFixedWidth(50);
    gLayout->addWidget(rgbG);
    gSlider = new QSlider(Qt::Horizontal, this);
    gSlider->setRange(0, 255);
    gLayout->addWidget(gSlider);
    rgbLayout->addLayout(gLayout);

    QHBoxLayout* bLayout = new QHBoxLayout();
    bLayout->addWidget(new QLabel("B:"));
    rgbB = new QLineEdit("0", this);
    rgbB->setFixedWidth(50);
    bLayout->addWidget(rgbB);
    bSlider = new QSlider(Qt::Horizontal, this);
    bSlider->setRange(0, 255);
    bLayout->addWidget(bSlider);
    rgbLayout->addLayout(bLayout);

    //rgbLayout->addStretch();
    columnsLayout->addWidget(rgbGroup);

    QGroupBox* labGroup = new QGroupBox("LAB", this);   // lab
    QVBoxLayout* labLayout = new QVBoxLayout(labGroup);

    QHBoxLayout* lLayout = new QHBoxLayout();
    lLayout->addWidget(new QLabel("L:"));
    labL = new QLineEdit("0", this);
    labL->setFixedWidth(50);
    lLayout->addWidget(labL);
    lSlider = new QSlider(Qt::Horizontal, this);
    lSlider->setRange(0, 100);
    lLayout->addWidget(lSlider);
    labLayout->addLayout(lLayout);

    QHBoxLayout* aLayout = new QHBoxLayout();
    aLayout->addWidget(new QLabel("a:"));
    labA = new QLineEdit("0", this);
    labA->setFixedWidth(50);
    aLayout->addWidget(labA);
    aSlider = new QSlider(Qt::Horizontal, this);
    aSlider->setRange(-128, 127);
    aLayout->addWidget(aSlider);
    labLayout->addLayout(aLayout);

    // b
    QHBoxLayout* bLLayout = new QHBoxLayout();
    bLLayout->addWidget(new QLabel("b:"));
    labB = new QLineEdit("0", this);
    labB->setFixedWidth(50);
    bLLayout->addWidget(labB);
    bLSlider = new QSlider(Qt::Horizontal, this);
    bLSlider->setRange(-128, 127);
    bLLayout->addWidget(bLSlider);
    labLayout->addLayout(bLLayout);

    //labLayout->addStretch();
    columnsLayout->addWidget(labGroup);

    QGroupBox* cmykGroup = new QGroupBox("CMYK", this);      //cmyk
    QVBoxLayout* cmykLayout = new QVBoxLayout(cmykGroup);

    QHBoxLayout* cLayout = new QHBoxLayout();
    cLayout->addWidget(new QLabel("C:"));
    cmykC = new QLineEdit("0", this);
    cmykC->setFixedWidth(50);
    cLayout->addWidget(cmykC);
    cSlider = new QSlider(Qt::Horizontal, this);
    cSlider->setRange(0, 100);
    cLayout->addWidget(cSlider);
    cmykLayout->addLayout(cLayout);

    QHBoxLayout* mLayout = new QHBoxLayout();
    mLayout->addWidget(new QLabel("M:"));
    cmykM = new QLineEdit("0", this);
    cmykM->setFixedWidth(47);
    mLayout->addWidget(cmykM);
    mSlider = new QSlider(Qt::Horizontal, this);
    mSlider->setRange(0, 100);
    mLayout->addWidget(mSlider);
    cmykLayout->addLayout(mLayout);

    QHBoxLayout* yLayout = new QHBoxLayout();
    yLayout->addWidget(new QLabel("Y:"));
    cmykY = new QLineEdit("0", this);
    cmykY->setFixedWidth(50);
    yLayout->addWidget(cmykY);
    ySlider = new QSlider(Qt::Horizontal, this);
    ySlider->setRange(0, 100);
    yLayout->addWidget(ySlider);
    cmykLayout->addLayout(yLayout);

    QHBoxLayout* kLayout = new QHBoxLayout();
    kLayout->addWidget(new QLabel("K:"));
    cmykK = new QLineEdit("0", this);
    cmykK->setFixedWidth(50);
    kLayout->addWidget(cmykK);
    kSlider = new QSlider(Qt::Horizontal, this);
    kSlider->setRange(0, 100);
    kLayout->addWidget(kSlider);
    cmykLayout->addLayout(kLayout);

    //cmykLayout->addStretch();
    columnsLayout->addWidget(cmykGroup);

    mainLayout->addLayout(columnsLayout);

    QGroupBox* colorGroup = new QGroupBox("Цвет", this);
    QVBoxLayout* colorLayout = new QVBoxLayout(colorGroup);

    colorDisplay = new QWidget(this);
    colorDisplay->setFixedHeight(300);
    colorDisplay->setStyleSheet("border: 1px solid gray;");
    colorLayout->addWidget(colorDisplay);

    mainLayout->addWidget(colorGroup);
}

void MainWindow::setupConnections() {
    connect(rgbR, SIGNAL(editingFinished()), this, SLOT(onRGBEdited()));
    connect(rgbG, SIGNAL(editingFinished()), this, SLOT(onRGBEdited()));
    connect(rgbB, SIGNAL(editingFinished()), this, SLOT(onRGBEdited()));

    connect(rSlider, SIGNAL(valueChanged(int)), this, SLOT(onRSliderChanged(int)));
    connect(gSlider, SIGNAL(valueChanged(int)), this, SLOT(onGSliderChanged(int)));
    connect(bSlider, SIGNAL(valueChanged(int)), this, SLOT(onBSliderChanged(int)));

    connect(labL, SIGNAL(editingFinished()), this, SLOT(onLABEdited()));
    connect(labA, SIGNAL(editingFinished()), this, SLOT(onLABEdited()));
    connect(labB, SIGNAL(editingFinished()), this, SLOT(onLABEdited()));

    connect(lSlider, SIGNAL(valueChanged(int)), this, SLOT(onLSliderChanged(int)));
    connect(aSlider, SIGNAL(valueChanged(int)), this, SLOT(onASliderChanged(int)));
    connect(bLSlider, SIGNAL(valueChanged(int)), this, SLOT(onBLSliderChanged(int)));

    connect(cmykC, SIGNAL(editingFinished()), this, SLOT(onCMYKEdited()));
    connect(cmykM, SIGNAL(editingFinished()), this, SLOT(onCMYKEdited()));
    connect(cmykY, SIGNAL(editingFinished()), this, SLOT(onCMYKEdited()));
    connect(cmykK, SIGNAL(editingFinished()), this, SLOT(onCMYKEdited()));

    connect(cSlider, SIGNAL(valueChanged(int)), this, SLOT(onCSliderChanged(int)));
    connect(mSlider, SIGNAL(valueChanged(int)), this, SLOT(onMSliderChanged(int)));
    connect(ySlider, SIGNAL(valueChanged(int)), this, SLOT(onYSliderChanged(int)));
    connect(kSlider, SIGNAL(valueChanged(int)), this, SLOT(onKSliderChanged(int)));

    connect(StandartBox, SIGNAL(currentIndexChanged(int)),this, SLOT(onStandartChanged(int)));
    connect(colorChoose, SIGNAL(clicked()), this, SLOT(onColorChooseClicked()));
    connect(tests, SIGNAL(clicked()), this, SLOT(onTestsClicked()));
}

void MainWindow::onRGBEdited() {
    if (updating) return;

    bool ok1, ok2, ok3;
    int r = rgbR->text().toInt(&ok1);
    int g = rgbG->text().toInt(&ok2);
    int b = rgbB->text().toInt(&ok3);

    if (!ok1 || !ok2 || !ok3) {
        showWarning("Введите целые числа");
        return;
    }

    bool clamped = false;
    if (r < 0) { r = 0; clamped = true; }
    if (r > 255) { r = 255; clamped = true; }
    if (g < 0) { g = 0; clamped = true; }
    if (g > 255) { g = 255; clamped = true; }
    if (b < 0) { b = 0; clamped = true; }
    if (b > 255) { b = 255; clamped = true; }

    if (clamped) {
        showWarning("RGB значения обрезаны (0-255)");
        rgbR->setText(QString::number(r));
        rgbG->setText(QString::number(g));
        rgbB->setText(QString::number(b));
    }

    controller->updateFromRGB(r, g, b);
}

void MainWindow::onLABEdited() {
    if (updating) return;

    bool ok1, ok2, ok3;
    double l = labL->text().toDouble(&ok1);
    double a = labA->text().toDouble(&ok2);
    double b = labB->text().toDouble(&ok3);

    if (!ok1 || !ok2 || !ok3) {
        showWarning("Введите корректные числа");
        return;
    }

    bool clamped = false;
    if (l < 0) { l = 0; clamped = true; }
    if (l > 100) { l = 100; clamped = true; }
    if (a < -128) { a = -128; clamped = true; }
    if (a > 127) { a = 127; clamped = true; }
    if (b < -128) { b = -128; clamped = true; }
    if (b > 127) { b = 127; clamped = true; }

    if (clamped) {
        showWarning("LAB значения обрезаны: L(0-100), a(-128-127), b(-128-127)");
        labL->setText(QString::number(l, 'f', 2));
        labA->setText(QString::number(a, 'f', 2));
        labB->setText(QString::number(b, 'f', 2));
    }

    controller->updateFromLAB(l, a, b);
}

void MainWindow::onCMYKEdited() {
    if (updating) return;

    bool ok1, ok2, ok3, ok4;
    double c = cmykC->text().toDouble(&ok1);
    double m = cmykM->text().toDouble(&ok2);
    double y = cmykY->text().toDouble(&ok3);
    double k = cmykK->text().toDouble(&ok4);

    if (!ok1 || !ok2 || !ok3 || !ok4) {
        showWarning("Введите корректные числа");
        return;
    }

    bool clamped = false;
    if (c < 0) { c = 0; clamped = true; }
    if (c > 100) { c = 100; clamped = true; }
    if (m < 0) { m = 0; clamped = true; }
    if (m > 100) { m = 100; clamped = true; }
    if (y < 0) { y = 0; clamped = true; }
    if (y > 100) { y = 100; clamped = true; }
    if (k < 0) { k = 0; clamped = true; }
    if (k > 100) { k = 100; clamped = true; }

    if (clamped) {
        showWarning("CMYK значения обрезаны (0-100%)");
        cmykC->setText(QString::number(c, 'f', 1));
        cmykM->setText(QString::number(m, 'f', 1));
        cmykY->setText(QString::number(y, 'f', 1));
        cmykK->setText(QString::number(k, 'f', 1));
    }

    controller->updateFromCMYK(c, m, y, k);
}

void MainWindow::onRSliderChanged(int r) {
    if (updating) return;
    int g = rgbG->text().toInt();
    int b = rgbB->text().toInt();
    controller->updateFromRGB(r, g, b);
}

void MainWindow::onGSliderChanged(int g) {
    if (updating) return;
    int r = rgbR->text().toInt();
    int b = rgbB->text().toInt();
    controller->updateFromRGB(r, g, b);
}

void MainWindow::onBSliderChanged(int b) {
    if (updating) return;
    int r = rgbR->text().toInt();
    int g = rgbG->text().toInt();
    controller->updateFromRGB(r, g, b);
}

void MainWindow::onLSliderChanged(int l) {
    if (updating) return;
    double a = labA->text().toDouble();
    double b = labB->text().toDouble();
    controller->updateFromLAB(l, a, b);
}

void MainWindow::onASliderChanged(int a) {
    if (updating) return;
    double l = labL->text().toDouble();
    double b = labB->text().toDouble();
    controller->updateFromLAB(l, a, b);
}

void MainWindow::onBLSliderChanged(int b) {
    if (updating) return;
    double l = labL->text().toDouble();
    double a = labA->text().toDouble();
    controller->updateFromLAB(l, a, b);
}

void MainWindow::onCSliderChanged(int c) {
    if (updating) return;
    double m = cmykM->text().toDouble();
    double y = cmykY->text().toDouble();
    double k = cmykK->text().toDouble();
    controller->updateFromCMYK(c, m, y, k);
}

void MainWindow::onMSliderChanged(int m) {
    if (updating) return;
    double c = cmykC->text().toDouble();
    double y = cmykY->text().toDouble();
    double k = cmykK->text().toDouble();
    controller->updateFromCMYK(c, m, y, k);
}

void MainWindow::onYSliderChanged(int y) {
    if (updating) return;
    double c = cmykC->text().toDouble();
    double m = cmykM->text().toDouble();
    double k = cmykK->text().toDouble();
    controller->updateFromCMYK(c, m, y, k);
}

void MainWindow::onKSliderChanged(int k) {
    if (updating) return;
    double c = cmykC->text().toDouble();
    double m = cmykM->text().toDouble();
    double y = cmykY->text().toDouble();
    controller->updateFromCMYK(c, m, y, k);
}

void MainWindow::onStandartChanged(int index) {
    controller->changeStandart(index);
}

void MainWindow::updateRGB(int r, int g, int b) {
    updating = true;
    rgbR->setText(QString::number(r));
    rgbG->setText(QString::number(g));
    rgbB->setText(QString::number(b));

    rSlider->setValue(r);
    gSlider->setValue(g);
    bSlider->setValue(b);

    updateSliderStyles(r, g, b);

    updating = false;
}

void MainWindow::updateLAB(double L, double a, double b) {
    updating = true;
    labL->setText(QString::number(L, 'f', 2));
    labA->setText(QString::number(a, 'f', 2));
    labB->setText(QString::number(b, 'f', 2));

    lSlider->setValue(qRound(L));
    aSlider->setValue(qRound(a));
    bLSlider->setValue(qRound(b));

    int r = rgbR->text().toInt();
    int g = rgbG->text().toInt();
    int bVal = rgbB->text().toInt();
    updateSliderStyles(r, g, bVal);

    updating = false;
}

void MainWindow::updateCMYK(double c, double m, double y, double k) {
    updating = true;
    cmykC->setText(QString::number(c, 'f', 1));
    cmykM->setText(QString::number(m, 'f', 1));
    cmykY->setText(QString::number(y, 'f', 1));
    cmykK->setText(QString::number(k, 'f', 1));

    cSlider->setValue(qRound(c));
    mSlider->setValue(qRound(m));
    ySlider->setValue(qRound(y));
    kSlider->setValue(qRound(k));

    int r = rgbR->text().toInt();
    int g = rgbG->text().toInt();
    int bVal = rgbB->text().toInt();
    updateSliderStyles(r, g, bVal);

    updating = false;
}

void MainWindow::updateColor(int r, int g, int b) {
    QString style = QString("background-color: rgb(%1,%2,%3); border: 1px solid gray;")
    .arg(r).arg(g).arg(b);
    colorDisplay->setStyleSheet(style);
}

void MainWindow::getCurrentRGB(int& r, int& g, int& b) {
    r = rgbR->text().toInt();
    g = rgbG->text().toInt();
    b = rgbB->text().toInt();
}

void MainWindow::showWarning(const QString& message) {
    QMessageBox::warning(this, "Ошибка", message);
}

void MainWindow::updateSliderStyles(int r, int g, int b) {
    QString rGradient = QString(
    "QSlider::groove:horizontal {"
    "height: 8px;"
    "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "stop:0 rgb(0, %1, %2),"
    "stop:1 rgb(255, %1, %2));"
    "border-radius: 4px;"
    "}"
    "QSlider::handle:horizontal {"
    "width: 18px; height: 18px;"
    "background: white;"
    "border: 2px solid #ff0000;"
    "border-radius: 9px;"
    "}").arg(g).arg(b);

    rSlider->setStyleSheet(rGradient);

    QString gGradient = QString(
    "QSlider::groove:horizontal {"
    "height: 8px;"
    "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "stop:0 rgb(%1, 0, %2),"
    "stop:1 rgb(%1, 255, %2));"
    "border-radius: 4px;"
    "}"
    "QSlider::handle:horizontal {"
    "width: 18px; height: 18px;"
    "background: white;"
    "border: 2px solid #00cc00;"
    "border-radius: 9px;"
    "}").arg(r).arg(b);

    gSlider->setStyleSheet(gGradient);

    QString bGradient = QString(
    "QSlider::groove:horizontal {"
    "height: 8px;"
    "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "stop:0 rgb(%1, %2, 0),"
    "stop:1 rgb(%1, %2, 255));"
    "border-radius: 4px;"
    "}"
    "QSlider::handle:horizontal {"
    "width: 18px; height: 18px;"
    "background: white;"
    "border: 2px solid #0000cc;"
    "border-radius: 9px;"
    "}").arg(r).arg(g);

    bSlider->setStyleSheet(bGradient);

    QString lGradient = QString(
    "QSlider::groove:horizontal {"
    "height: 8px;"
    "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "stop:0 rgb(0, 0, 0),"
    "stop:1 rgb(255, 255, 255));"
    "border-radius: 4px;"
    "}"
    "QSlider::handle:horizontal {"
    "width: 18px; height: 18px;"
    "background: white;"
    "border: 2px solid #666666;"
    "border-radius: 9px;"
    "}");

    lSlider->setStyleSheet(lGradient);

    QString aGradient = QString(
    "QSlider::groove:horizontal {"
    "height: 8px;"
    "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "stop:0 rgb(0, 200, 0),"
    "stop:0.5 rgb(128, 128, 128),"
    "stop:1 rgb(200, 0, 0));"
    "border-radius: 4px;"
    "}"
    "QSlider::handle:horizontal {"
    "width: 18px; height: 18px;"
    "background: white;"
    "border: 2px solid #888888;"
    "border-radius: 9px;"
    "}");

    aSlider->setStyleSheet(aGradient);

    QString bLabGradient = QString(
    "QSlider::groove:horizontal {"
    "height: 8px;"
    "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "stop:0 rgb(0, 0, 200),"
    "stop:0.5 rgb(128, 128, 128),"
    "stop:1 rgb(200, 200, 0));"
    "border-radius: 4px;"
    "}"
    "QSlider::handle:horizontal {"
    "width: 18px; height: 18px;"
    "background: white;"
    "border: 2px solid #888888;"
    "border-radius: 9px;"
    "}");

    bLSlider->setStyleSheet(bLabGradient);

    QString cGradient = QString(
    "QSlider::groove:horizontal {"
    "height: 8px;"
    "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "stop:0 rgb(255, 255, 255),"
    "stop:1 rgb(0, 200, 255));"
    "border-radius: 4px;"
    "}"
    "QSlider::handle:horizontal {"
    "width: 18px; height: 18px;"
    "background: white;"
    "border: 2px solid #00aadd;"
    "border-radius: 9px;"
    "}");
    cSlider->setStyleSheet(cGradient);

    QString mGradient = QString(
    "QSlider::groove:horizontal {"
    "height: 8px;"
    "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "stop:0 rgb(255, 255, 255),"
    "stop:1 rgb(255, 0, 255));"
    "border-radius: 4px;"
    "}"
    "QSlider::handle:horizontal {"
    "width: 18px; height: 18px;"
    "background: white;"
    "border: 2px solid #cc00cc;"
    "border-radius: 9px;"
    "}");

    mSlider->setStyleSheet(mGradient);

    QString yGradient = QString(
    "QSlider::groove:horizontal {"
    "height: 8px;"
    "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "stop:0 rgb(255, 255, 255),"
    "stop:1 rgb(255, 255, 0));"
    "border-radius: 4px;"
    "}"
    "QSlider::handle:horizontal {"
    "width: 18px; height: 18px;"
    "background: white;"
    "border: 2px solid #ccaa00;"
    "border-radius: 9px;"
    "}");

    ySlider->setStyleSheet(yGradient);

    QString kGradient = QString(
    "QSlider::groove:horizontal {"
    "height: 8px;"
    "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "stop:0 rgb(255, 255, 255),"
    "stop:1 rgb(0, 0, 0));"
    "border-radius: 4px;"
    "}"
    "QSlider::handle:horizontal {"
    "width: 18px; height: 18px;"
    "background: white;"
    "border: 2px solid #888888;"
    "border-radius: 9px;"
    "}");

    kSlider->setStyleSheet(kGradient);
}

void MainWindow::onColorChooseClicked(){
    QColor color = QColorDialog::getColor();
    if (color.isValid()) {
        int r, g, b;
        color.getRgb(&r, &g, &b);
        controller->updateFromRGB(r, g, b);
    }
}

void MainWindow::onTestsClicked(){
    bool flag = 0;
    controller->updateFromRGB(255, 0, 0);

    double red_L, red_a, red_b;
    double green_L, green_a, green_b;
    double blue_L, blue_a, blue_b;

    int standart = ColorModel::getStandart();
    if (standart == 0) {  //D65
        red_L = 53.24;   red_a = 80.09;   red_b = 67.20;
        green_L = 87.73; green_a = -86.18; green_b = 83.18;
        blue_L = 32.30;  blue_a = 79.19;  blue_b = -107.86;
    } else if (standart == 1) {  // D50
        red_L = 54.29;   red_a = 80.81;   red_b = 69.88;
        green_L = 87.82; green_a = -79.29; green_b = 80.99;
        blue_L = 29.57;  blue_a = 68.30;  blue_b = -112.03;
    }
    else {  // E
        red_L = 53.24;   red_a = 73.74;   red_b = 65.90;
        green_L = 87.86; green_a = -92.35; green_b = 80.29;
        blue_L = 32.20;  blue_a = 74.42;  blue_b = -113.38;
    }


    if (qAbs(labL->text().toDouble() - red_L) > 0.5) flag = 1;
    if (qAbs(labA->text().toDouble() - red_a) > 0.5) flag = 1;
    if (qAbs(labB->text().toDouble() - red_b) > 0.5) flag = 1;
    if (qAbs(cmykC->text().toDouble() - 0.0) > 0.5) flag = 1;
    if (qAbs(cmykM->text().toDouble() - 100.0) > 0.5) flag = 1;
    if (qAbs(cmykY->text().toDouble() - 100.0) > 0.5) flag = 1;
    if (qAbs(cmykK->text().toDouble() - 0.0) > 0.5) flag = 1;

    if (flag == 1) {
        showWarning(QString("Тест номер 1 не прошел"));
    }

    flag = 0;
    controller->updateFromRGB(0, 255, 0);

    if (qAbs(labL->text().toDouble() - green_L) > 0.5) flag = 1;
    if (qAbs(labA->text().toDouble() - green_a) > 0.5) flag = 1;
    if (qAbs(labB->text().toDouble() - green_b) > 0.5) flag = 1;
    if (qAbs(cmykC->text().toDouble() - 100.0) > 0.5) flag = 1;
    if (qAbs(cmykM->text().toDouble() - 0.0) > 0.5) flag = 1;
    if (qAbs(cmykY->text().toDouble() - 100.0) > 0.5) flag = 1;
    if (qAbs(cmykK->text().toDouble() - 0.0) > 0.5) flag = 1;

    if (flag == 1) {
        showWarning(QString("Тест номер 2 не прошел"));
    }

    flag = 0;
    controller->updateFromRGB(0, 0, 255);

    if (qAbs(labL->text().toDouble() - blue_L) > 0.5) flag = 1;
    if (qAbs(labA->text().toDouble() - blue_a) > 0.5) flag = 1;
    if (qAbs(labB->text().toDouble() - blue_b) > 0.5) flag = 1;
    if (qAbs(cmykC->text().toDouble() - 100.0) > 0.5) flag = 1;
    if (qAbs(cmykM->text().toDouble() - 100.0) > 0.5) flag = 1;
    if (qAbs(cmykY->text().toDouble() - 0.0) > 0.5) flag = 1;
    if (qAbs(cmykK->text().toDouble() - 0.0) > 0.5) flag = 1;

    if (flag == 1) {
        showWarning(QString("Тест номер 3 не прошел"));
    }

    flag = 0;
    controller->updateFromLAB(100, 0, 0);

    if (qAbs(rgbR->text().toInt() - 255) > 1) flag = 1;
    if (qAbs(rgbG->text().toInt() - 249) > 1) flag = 1;
    if (qAbs(rgbB->text().toInt() - 244) > 1) flag = 1;
    if (qAbs(cmykC->text().toDouble() - 0.0) > 0.5) flag = 1;
    if (qAbs(cmykM->text().toDouble() - 0.0) > 2.5) flag = 1;
    if (qAbs(cmykY->text().toDouble() - 0.0) > 5.0) flag = 1;
    if (qAbs(cmykK->text().toDouble() - 0.0) > 0.5) flag = 1;

    if (flag == 1) {
        showWarning(QString("Тест номер 4 не прошел"));
    }

    flag = 0;
    controller->updateFromLAB(0, 0, 0);

    if (qAbs(rgbR->text().toInt() - 0) > 1) flag = 1;
    if (qAbs(rgbG->text().toInt() - 0) > 1) flag = 1;
    if (qAbs(rgbB->text().toInt() - 0) > 1) flag = 1;
    if (qAbs(cmykC->text().toDouble() - 0.0) > 0.5) flag = 1;
    if (qAbs(cmykM->text().toDouble() - 0.0) > 0.5) flag = 1;
    if (qAbs(cmykY->text().toDouble() - 0.0) > 0.5) flag = 1;
    if (qAbs(cmykK->text().toDouble() - 100.0) > 0.5) flag = 1;

    if (flag == 1) {
        showWarning(QString("Тест номер 5 не прошел"));
    }

    flag = 0;
    controller->updateFromLAB(red_L, red_a, red_b);

    if (qAbs(rgbR->text().toInt() - 255) > 2) flag = 1;
    if (qAbs(rgbG->text().toInt() - 0) > 2) flag = 1;
    if (qAbs(rgbB->text().toInt() - 0) > 2) flag = 1;
    if (qAbs(cmykC->text().toDouble() - 0.0) > 0.5) flag = 1;
    if (qAbs(cmykM->text().toDouble() - 100.0) > 0.5) flag = 1;
    if (qAbs(cmykY->text().toDouble() - 100.0) > 0.5) flag = 1;
    if (qAbs(cmykK->text().toDouble() - 0.0) > 0.5) flag = 1;

    if (flag == 1) {
        showWarning(QString("Тест номер 6 не прошел"));
    }

    flag = 0;
    controller->updateFromCMYK(0, 0, 0, 0);

    if (qAbs(rgbR->text().toInt() - 255) > 1) flag = 1;
    if (qAbs(rgbG->text().toInt() - 255) > 1) flag = 1;
    if (qAbs(rgbB->text().toInt() - 255) > 1) flag = 1;
    if (qAbs(labL->text().toDouble() - 100.0) > 0.5) flag = 1;
    if (qAbs(labA->text().toDouble() - 0.0) > 10.0) flag = 1;
    if (qAbs(labB->text().toDouble() - 0.0) > 10.0) flag = 1;

    if (flag == 1) {
        showWarning(QString("Тест номер 7 не прошел"));
    }

    flag = 0;
    controller->updateFromCMYK(0, 0, 0, 100);

    if (qAbs(rgbR->text().toInt() - 0) > 1) flag = 1;
    if (qAbs(rgbG->text().toInt() - 0) > 1) flag = 1;
    if (qAbs(rgbB->text().toInt() - 0) > 1) flag = 1;
    if (qAbs(labL->text().toDouble() - 0.0) > 0.5) flag = 1;
    if (qAbs(labA->text().toDouble() - 0.0) > 0.5) flag = 1;
    if (qAbs(labB->text().toDouble() - 0.0) > 0.5) flag = 1;

    if (flag == 1) {
        showWarning(QString("Тест номер 8 не прошел"));
    }

    flag = 0;
    controller->updateFromCMYK(0, 100, 100, 0);

    if (qAbs(rgbR->text().toInt() - 255) > 2) flag = 1;
    if (qAbs(rgbG->text().toInt() - 0) > 2) flag = 1;
    if (qAbs(rgbB->text().toInt() - 0) > 2) flag = 1;
    if (qAbs(labL->text().toDouble() - red_L) > 0.5) flag = 1;
    if (qAbs(labA->text().toDouble() - red_a) > 0.5) flag = 1;
    if (qAbs(labB->text().toDouble() - red_b) > 0.5) flag = 1;

    if (flag == 1) {
        showWarning(QString("Тест номер 9 не прошел"));
    }

    if(flag == 0){
        showWarning("Все тесты пройдены успешно!");
    }
}