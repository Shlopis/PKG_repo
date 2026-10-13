#include "mainwindow.h"
#include "histogramwidget.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QDebug>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Image Processing Lab - Шаг 3");
    resize(1000, 700);

    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    imageLabel->setText("Загрузите изображение через меню Файл -> Открыть");
    imageLabel->setStyleSheet("background-color: #f0f0f0; border: 1px solid #ccc;");

    scrollArea = new QScrollArea(this);
    scrollArea->setWidget(imageLabel);
    scrollArea->setAlignment(Qt::AlignCenter);

    histogramWidget = new HistogramWidget(this);
    histogramWidget->setMinimumHeight(150);
    histogramWidget->setMaximumHeight(200);
    applied_histogramWidget = new HistogramWidget(this);
    applied_histogramWidget->setMinimumHeight(150);
    applied_histogramWidget->setMaximumHeight(200);

    btnLinearContrast = new QPushButton("Линейное контрастирование", this);
    btnHistEqualization = new QPushButton("Выравнивание гистограммы(Grayscale)", this);
    btnLinearContrast->setEnabled(false);
    btnHistEqualization->setEnabled(false);

    btnHistEqRGB = new QPushButton("Выравнивание (RGB)", this);
    btnHistEqHSV = new QPushButton("Выравнивание (HSV)", this);
    btnHistEqRGB->setEnabled(false);
    btnHistEqHSV->setEnabled(false);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(btnLinearContrast);
    buttonsLayout->addWidget(btnHistEqualization);

    buttonsLayout->addWidget(btnHistEqRGB);
    buttonsLayout->addWidget(btnHistEqHSV);

    btnInversion = new QPushButton("Инверсия", this);
    btnThreshold = new QPushButton("Порог (128)", this);
    btnGamma = new QPushButton("Гамма (0.5)", this);
    btnInversion->setEnabled(false);
    btnThreshold->setEnabled(false);
    btnGamma->setEnabled(false);

    QHBoxLayout* buttonsLayout2 = new QHBoxLayout;
    buttonsLayout2->addWidget(btnInversion);
    buttonsLayout2->addWidget(btnThreshold);
    buttonsLayout2->addWidget(btnGamma);

    progressBar = new QProgressBar(this);
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);

    QVBoxLayout* mainLayout = new QVBoxLayout;

    QWidget* centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    mainLayout->addWidget(scrollArea, 1);
    mainLayout->addWidget(histogramWidget);
    mainLayout->addWidget(applied_histogramWidget);
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addLayout(buttonsLayout2);
    mainLayout->addWidget(progressBar);

    btnCLAHE = new QPushButton("CLAHE", this);
    btnCLAHE->setEnabled(false);
    buttonsLayout2->addWidget(btnCLAHE);

    connect(btnCLAHE, &QPushButton::clicked, this, &MainWindow::onCLAHE);
    connect(btnInversion, &QPushButton::clicked, this, &MainWindow::onInversion);
    connect(btnThreshold, &QPushButton::clicked, this, &MainWindow::onThreshold);
    connect(btnGamma, &QPushButton::clicked, this, &MainWindow::onGamma);
    connect(btnHistEqRGB, &QPushButton::clicked, this, &MainWindow::onHistEqRGB);
    connect(btnHistEqHSV, &QPushButton::clicked, this, &MainWindow::onHistEqHSV);
    connect(btnLinearContrast, &QPushButton::clicked, this, &MainWindow::onLinearContrast);
    connect(btnHistEqualization, &QPushButton::clicked, this, &MainWindow::onHistEqualization);
    connect(btnGamma, &QPushButton::clicked, this, &MainWindow::onGamma);
    connect(btnInversion, &QPushButton::clicked, this, &MainWindow::onInversion);
    connect(btnThreshold, &QPushButton::clicked, this, &MainWindow::onThreshold);

    watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, this, &MainWindow::onProcessingFinished);

    createActions();
}

void MainWindow::createActions() {
    QMenu* fileMenu = menuBar()->addMenu("Файл");
    QAction* openAction = fileMenu->addAction("Открыть изображение...");
    connect(openAction, &QAction::triggered, this, &MainWindow::openImage);
}

void MainWindow::openImage() {
    QString filePath = QFileDialog::getOpenFileName(this, "Открыть изображение", "", "Images (*.png *.jpg *.bmp *.jpeg *.tiff)");
    if (filePath.isEmpty()) return;

    QImage testLoad(filePath);
    if (testLoad.isNull()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить изображение");
        return;
    }

    qDebug() << "=== ИНФОРМАЦИЯ ОБ ИЗОБРАЖЕНИИ ===";
    qDebug() << "Файл:" << filePath;
    qDebug() << "Размер:" << testLoad.width() << "x" << testLoad.height();
    qDebug() << "Формат:" << testLoad.format();
    qDebug() << "===================================";

    if (!wrapper.loadImage(filePath)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить изображение");
        return;
    }

    linearApplied = false;
    histGrayApplied = false;
    histColorApplied = false;

    updateImageDisplay();
    updateButtonsState();
}

void MainWindow::updateImageDisplay() {
    QPixmap pixmap = QPixmap::fromImage(wrapper.qImage());
    imageLabel->setPixmap(pixmap);
    imageLabel->adjustSize();

    HistogramData histData = ImageProcessor::calculateHistogram(wrapper, 4);
    if(histColorApplied || histGrayApplied)
        applied_histogramWidget->setHistogramData(histData, wrapper.isGrayscale());
    else{
        histogramWidget->setHistogramData(histData, wrapper.isGrayscale());
    }
}

void MainWindow::startProcessing(std::function<void()> task) {
    if (isProcessing) return;

    isProcessing = true;
    updateButtonsState();

    progressBar->setMinimum(0);
    progressBar->setMaximum(0);
    progressBar->setValue(0);

    statusBar()->showMessage("Обработка...");

    QFuture<void> future = QtConcurrent::run(task);
    watcher->setFuture(future);
}
void MainWindow::onProcessingFinished() {
    isProcessing = false;

    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(100);

    updateImageDisplay();
    updateButtonsState();
    statusBar()->showMessage("Обработка завершена", 2000);
}

void MainWindow::onLinearContrast() {
    startProcessing([this]() {
         ImageProcessor::applyLinearContrast(wrapper, 4);
   });
    linearApplied = true;
}

void MainWindow::onHistEqualization() {
    startProcessing([this]() {
        ImageProcessor::applyHistogramEqualization(wrapper, 4);
    });
    histGrayApplied = true;
}

void MainWindow::onHistEqRGB() {
    startProcessing([this]() {
        ImageProcessor::applyHistogramEqualizationRGB(wrapper, 4);
    });
    histColorApplied = true;
}

void MainWindow::onHistEqHSV() {
    startProcessing([this]() {
        ImageProcessor::applyHistogramEqualizationHSV(wrapper, 4);
    });
    histColorApplied = true;
}
void MainWindow::updateButtonsState() {
    bool hasImage = (wrapper.width() > 0 && wrapper.height() > 0);
    bool isGray = wrapper.isGrayscale();

    btnLinearContrast->setEnabled(hasImage && !linearApplied && !isProcessing);
    btnHistEqualization->setEnabled(hasImage && isGray && !histGrayApplied && !isProcessing);
    btnHistEqRGB->setEnabled(hasImage && !isGray && !histColorApplied && !isProcessing);
    btnHistEqHSV->setEnabled(hasImage && !isGray && !histColorApplied && !isProcessing);

    btnInversion->setEnabled(hasImage && !isProcessing);
    btnThreshold->setEnabled(hasImage && isGray && !isProcessing);
    btnGamma->setEnabled(hasImage && !isProcessing);
    btnCLAHE->setEnabled(hasImage && !isProcessing);
}
void MainWindow::onInversion() {
    startProcessing([this]() {
        ImageProcessor::applyInversion(wrapper, 4);
    });
}

void MainWindow::onThreshold() {
    startProcessing([this]() {
        ImageProcessor::applyThreshold(wrapper, 128, 4);
    });
}

void MainWindow::onGamma() {
    startProcessing([this]() {
        ImageProcessor::applyGammaCorrection(wrapper, 0.5, 4);
    });
}
void MainWindow::onCLAHE() {
    startProcessing([this]() {
        if (wrapper.isGrayscale()) {
            ImageProcessor::applyCLAHE(wrapper, 8, 8, 2.0, 4);
            histGrayApplied= true;
        } else {
            ImageProcessor::applyCLAHEColor(wrapper, 8, 8, 2.0, 4);
            histColorApplied = true;
        }
    });
}