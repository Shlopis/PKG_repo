#include "scanengine.h"
#include "parsers.h"

#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QTimer>


class ParseTask : public QRunnable {
public:
    ParseTask(ScanEngine *e, QString path) : engine(e), fPath(std::move(path)) { setAutoDelete(true); }

    void run() override
    {
        if (!engine->isCancelled()) {
            const ImageInfo info = ParserRegistry::parseFile(fPath);
            engine->reportResult(info);
        }
        engine->taskFinished();
    }

private:
    ScanEngine *engine = nullptr;
    QString fPath;
};


ScanEngine::ScanEngine(QObject *parent) : QObject(parent)
{
    const int threads = qMax(2, QThread::idealThreadCount());
    pool.setMaxThreadCount(threads);

    timer = new QTimer(this);
    timer->setInterval(120);
    timer->setSingleShot(false);
    connect(timer, &QTimer::timeout, this, &ScanEngine::flushResults);
}

ScanEngine::~ScanEngine()
{
    cancelled.store(true);
    pool.waitForDone();
}

void ScanEngine::startScan(const QString &folderPath)
{
    if (running)
        return;

    QStringList files;
    collectImageFiles(folderPath, files);

    if (files.isEmpty()) {
        emit scanFailed(QStringLiteral("В папке нет поддерживаемых файлов"));
        return;
    }

    running = true;
    cancelled.store(false);
    total = files.size();
    processed = 0;

    {
        QMutexLocker lock(&bufferMutex);
        buffer.clear();
        buffer.reserve(kBufferReserve);
    }

    pendingTasks.store(files.size());

    emit scanStarted(total);
    timer->start();

    for (const QString &path : files) {
        pool.start(new ParseTask(this, path));
    }
}

void ScanEngine::stopScan()
{
    cancelled.store(true);
}

void ScanEngine::reportResult(const ImageInfo &info)
{
    QMutexLocker lock(&bufferMutex);
    buffer.append(info);
}

void ScanEngine::taskFinished()
{
    const int left = pendingTasks.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (left == 0) {
        onAllTasksDone();
    }
}

void ScanEngine::flushResults()
{
    QVector<ImageInfo> batch;
    {
        QMutexLocker lock(&bufferMutex);
        if (buffer.isEmpty())
            return;
        batch.swap(buffer);
        buffer.reserve(kBufferReserve);
    }

    processed += batch.size();
    emit filesReady(batch);
    emit progressChanged(processed, total);
}

void ScanEngine::onAllTasksDone()
{
    timer->stop();
    flushResults();
    running = false;
    emit scanFinished();
}

void ScanEngine::collectImageFiles(const QString &folderPath, QStringList &out) const
{
    const QDir dir(folderPath);
    if (!dir.exists())
        return;

    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::NoSymLinks, QDir::NoSort);

    out.reserve(entries.size());
    for (const QFileInfo &fi : entries) {
        const QString path = fi.absoluteFilePath();
        if (ParserRegistry::isSupportedExtension(path))
            out.append(path);
    }
}

