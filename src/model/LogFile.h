#pragma once

#include <QObject>
#include <QFile>
#include <QString>

// Simple append-only logger.
// - File is created on first write() call (lazy open).
// - File lives at <applicationDirPath>/logs/<jobName>-<timestamp>.log
// - Keeps at most kMaxLogFiles files in the logs directory (oldest deleted first).
// - Each line is prefixed with [timestamp].
// - No buffering: every write() goes straight to disk.
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
    const bool exists() const { return m_file.exists(); }

signals:
    void error(const QString &message);

private:
    void cleanupOldLogs(const QString &logDirPath);
    QString sanitizeFileName(const QString &name);

    QString m_jobName;
    QFile m_file;
};
