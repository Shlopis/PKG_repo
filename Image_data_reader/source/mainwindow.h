#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include "imageinfo.h"
#include "imagetablemodel.h"
#include "scanengine.h"


class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onChooseFolder();
    void onStartScan();
    void onStopScan();

private:
    void setupUi();

    QTableView *table = nullptr;
    QProgressBar *progressBar = nullptr;
    QPushButton *chooseBtn = nullptr;
    QPushButton *startBtn = nullptr;
    QPushButton *stopBtn = nullptr;
    QLabel *folderLabel = nullptr;
    QLabel *statusLabel = nullptr;

    ImageTableModel *model = nullptr;
    ScanEngine *engineScan = nullptr;

    QString folderName;
};

#endif // MAINWINDOW_H
