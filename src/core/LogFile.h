#pragma once

#include <QFile>
#include <QObject>
#include <QString>
#include <QTimer>

// ---------------------------------------------------------------------------
// LogFile
// Simple append-only logger for a single job. Writes to
// <applicationDirPath>/logs/<jobName>-<timestamp>.log, created lazily on
// first write(); keeps only the newest Config::MAX_LOG_FILES per directory.
// Writes are buffered by QFile/the OS and flushed periodically (rather than
// after every line) since jobs can emit output rapidly during transfers;
// close() always flushes so nothing pending is lost.
// ---------------------------------------------------------------------------
class LogFile : public QObject
{
    Q_OBJECT

public:
    explicit LogFile(const QString &jobName, QObject *parent = nullptr);
    ~LogFile() override;

    void open();
    void write(const QString &line);
    void close();

    const QFile& file() const { return m_file; }
    bool exists() const { return m_file.exists(); }

signals:
    void error(const QString &message);

private:
    void cleanupOldLogs(const QString &logDirPath);
    QString sanitizeFileName(const QString &name);

    QString m_jobName;
    QFile m_file;
    QTimer m_flushTimer;
    bool m_pendingFlush = false;
};
