#ifndef SCANENGINE_H
#define SCANENGINE_H

#include <QObject>
#include <QThreadPool>
#include <QVector>
#include <QMutex>
#include <QStringList>
#include <atomic>

#include "imageinfo.h"

class QTimer;

class ScanEngine : public QObject {
    Q_OBJECT
public:
    explicit ScanEngine(QObject *parent = nullptr);
    ~ScanEngine() override;

    void reportResult(const ImageInfo &info);
    void taskFinished();

    bool isCancelled() const { return cancelled.load(); }
    bool isRunning() const { return running; }

public slots:
    void startScan(const QString &folderPath);
    void stopScan();

signals:
    void scanStarted(int totalFiles);
    void progressChanged(int processed, int total);
    void filesReady(const QVector<ImageInfo> &batch);
    void scanFinished();
    void scanFailed(const QString &reason);

private slots:
    void flushResults();       // по таймеру, в UI-потоке
    void onAllTasksDone();     // в UI-потоке, когда счётчик дошёл до 0

private:
    void collectImageFiles(const QString &folderPath, QStringList &out) const;

    QThreadPool pool;
    QTimer *timer = nullptr;

    mutable QMutex bufferMutex;
    QVector<ImageInfo> buffer;

    std::atomic<int> pendingTasks{0};
    std::atomic<bool> cancelled{false};
    bool running = false;
    int total = 0;
    int processed = 0;

    static constexpr int kBufferReserve = 256;
};




#endif // SCANENGINE_H
