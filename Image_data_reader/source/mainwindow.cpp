#include "mainwindow.h"
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUi();

    model = new ImageTableModel(this);
    engineScan = new ScanEngine(this);
    table->setModel(model);

    connect(chooseBtn, &QPushButton::clicked, this, &MainWindow::onChooseFolder);
    connect(startBtn,  &QPushButton::clicked, this, &MainWindow::onStartScan);
    connect(stopBtn,   &QPushButton::clicked, this, &MainWindow::onStopScan);

    connect(engineScan, &ScanEngine::scanStarted, this,
            [this](int total) {
                model->clear();
                progressBar->setMaximum(total > 0 ? total : 1);
                progressBar->setValue(0);
                statusLabel->setText(QString("Найдено файлов: %1").arg(total));
            });

    connect(engineScan, &ScanEngine::progressChanged, this,
            [this](int p, int t) {
                progressBar->setValue(p);
                statusLabel->setText(QString("Обработано: %1 / %2").arg(p).arg(t));
            });

    connect(engineScan, &ScanEngine::filesReady, this,
            [this](const QVector<ImageInfo> &batch) {
                model->appendBatch(batch);
            });

    connect(engineScan, &ScanEngine::scanFinished, this,
            [this]() {
                statusLabel->setText("Готово");
                startBtn->setEnabled(true);
                stopBtn->setEnabled(false);
            });

    connect(engineScan, &ScanEngine::scanFailed, this,
            [this](const QString &reason) {
                statusLabel->setText(QString("Ошибка: %1").arg(reason));
                startBtn->setEnabled(true);
                stopBtn->setEnabled(false);
            });
}

void MainWindow::setupUi()
{
    setWindowTitle("Image data");

    chooseBtn = new QPushButton(QString("Выбрать папку…"), this);
    startBtn = new QPushButton(QString("Старт"), this);
    stopBtn = new QPushButton(QString("Стоп"), this);
    stopBtn->setEnabled(false);

    folderLabel = new QLabel(QString("Папка не выбрана"), this);
    statusLabel = new QLabel(QString("Ожидание"), this);

    QHBoxLayout *top = new QHBoxLayout;
    top->addWidget(chooseBtn);
    top->addWidget(folderLabel);
    top->addWidget(startBtn);
    top->addWidget(stopBtn);

    table = new QTableView(this);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 1);
    progressBar->setValue(0);

    QHBoxLayout *bottom = new QHBoxLayout;
    bottom->addWidget(progressBar);
    bottom->addWidget(statusLabel);

    QWidget *central = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(central);
    root->addLayout(top);
    root->addWidget(table);
    root->addLayout(bottom);
    setCentralWidget(central);

    resize(1200, 800);
}

void MainWindow::onChooseFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Выберите папку"));
    if (dir.isEmpty())
        return;
    folderName = dir;
    folderLabel->setText(dir);
}

void MainWindow::onStartScan()
{
    if (folderName.isEmpty()) {
        statusLabel->setText(QString("Сначала выберите папку"));
        return;
    }
    if(engineScan->isRunning())
        return;
    startBtn->setEnabled(false);
    stopBtn->setEnabled(true);
    statusLabel->setText(QString("Сканирование…"));
    engineScan->startScan(folderName);
}

void MainWindow::onStopScan()
{
    engineScan->stopScan();
    stopBtn->setEnabled(false);
}