#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>
#include <QProgressBar>
#include <QFutureWatcher>
#include "imagewrapper.h"
#include "histogramwidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    QLabel* imageLabel;
    QScrollArea* scrollArea;
    QPushButton* btnLinearContrast;
    QPushButton* btnHistEqualization;
    QProgressBar* progressBar;
    QPushButton* btnHistEqRGB;
    QPushButton* btnHistEqHSV;
    QPushButton* btnInversion;
    QPushButton* btnThreshold;
    QPushButton* btnGamma;
    QPushButton* btnCLAHE;

    ImageWrapper wrapper;
    HistogramWidget* histogramWidget;
    HistogramWidget* applied_histogramWidget;

    QFutureWatcher<void>* watcher;

    void createActions();
    void updateImageDisplay();
    void startProcessing(std::function<void()> task);

    bool isProcessing = false;
    bool linearApplied = false;
    bool histGrayApplied = false;
    bool histColorApplied = false;

    void updateButtonsState();
private slots:
    void openImage();
    void onLinearContrast();
    void onHistEqualization();
    void onProcessingFinished();
    void onHistEqRGB();
    void onHistEqHSV();
    void onInversion();
    void onThreshold();
    void onGamma();
    void onCLAHE();

public:
    MainWindow(QWidget* parent = nullptr);
};

#endif // MAINWINDOW_H
