#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>

class ColorController;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updateRGB(int r, int g, int b);
    void updateLAB(double l, double a, double b);
    void updateCMYK(double c, double m, double y, double k);
    void updateColor(int r, int g, int b);
    void showWarning(const QString& message);
    void getCurrentRGB(int& r, int& g, int& b);

private slots:
    void onRGBEdited();
    void onLABEdited();
    void onCMYKEdited();
    void onStandartChanged(int index);
    void onRSliderChanged(int value);
    void onGSliderChanged(int value);
    void onBSliderChanged(int value);
    void onLSliderChanged(int value);
    void onASliderChanged(int value);
    void onBLSliderChanged(int value);
    void onCSliderChanged(int value);
    void onMSliderChanged(int value);
    void onYSliderChanged(int value);
    void onKSliderChanged(int value);
    void onColorChooseClicked();
    void onTestsClicked();

private:
    void setupUI();
    void setupConnections();
    void updateSliderStyles(int r, int g, int b);

    bool updating;

    ColorController* controller;
    QWidget* colorDisplay;
    QComboBox* StandartBox;

    QLineEdit* rgbR;
    QLineEdit* rgbG;
    QLineEdit* rgbB;
    QSlider* rSlider;
    QSlider* gSlider;
    QSlider* bSlider;

    QLineEdit* labL;
    QLineEdit* labA;
    QLineEdit* labB;
    QSlider* lSlider;
    QSlider* aSlider;
    QSlider* bLSlider;

    QLineEdit* cmykC;
    QLineEdit* cmykM;
    QLineEdit* cmykY;
    QLineEdit* cmykK;
    QSlider* cSlider;
    QSlider* mSlider;
    QSlider* ySlider;
    QSlider* kSlider;

    QPushButton* colorChoose;
    QPushButton* tests;
};
#endif // MAINWINDOW_H