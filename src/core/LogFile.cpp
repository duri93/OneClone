#include "LogFile.h"

#include "src/common/Config.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfoList>

LogFile::LogFile(const QString &jobName, QObject *parent)
    : QObject(parent)
    , m_jobName(jobName){

    // Batch flush()es instead of doing one after every write() — jobs can
    // emit output lines rapidly during transfers, and flush() is a real
    // per-line I/O cost.
    m_flushTimer.setInterval(Config::LOG_FLUSH_INTERVAL_MS);
    connect(&m_flushTimer, &QTimer::timeout, this, [this](){
        if (m_pendingFlush && m_file.isOpen()) {
            m_file.flush();
            m_pendingFlush = false;
        }
    });
}

LogFile::~LogFile(){
    close();
}

void LogFile::open(){
    const QString logDirPath = QCoreApplication::applicationDirPath() + "/logs";
    QDir logDir(logDirPath);

    if (!logDir.exists() && !logDir.mkpath(".")) {
        emit error(QStringLiteral("Failed to create log directory: %1").arg(logDirPath));
        return;
    }

    cleanupOldLogs(logDirPath);

    const QString fileName = QString("%1-%2.log").arg(
        sanitizeFileName(m_jobName),
        QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")
    );

    m_file.setFileName(logDir.filePath(fileName));

    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        emit error(QStringLiteral("Failed to open log file: %1").arg(m_file.errorString()));
    }
}
void LogFile::write(const QString &line){
    if (!m_file.isOpen()) open();
    if (!m_file.isOpen()) return; // error() already emitted by openFile()

    const QString entry = QString("[%1] %2\n").arg(
        QDateTime::currentDateTime().toString(Qt::ISODate),
        line
    );

    if (m_file.write(entry.toUtf8()) == -1) {
        emit error(QStringLiteral("Failed to write to log file: %1").arg(m_file.errorString()));
        return;
    }

    m_pendingFlush = true;
    if (!m_flushTimer.isActive()) {
        m_flushTimer.start();
    }
}

void LogFile::close(){
    m_flushTimer.stop();
    if (m_file.isOpen()){
        if (m_pendingFlush) {
            m_file.flush();
            m_pendingFlush = false;
        }
        m_file.close();
    }
}

void LogFile::cleanupOldLogs(const QString &logDirPath){
    QDir logDir(logDirPath);

    // Oldest first.
    QFileInfoList files = logDir.entryInfoList(
        QStringList() << "*.log",
        QDir::Files,
        QDir::Time | QDir::Reversed);

    // Make room so that after adding the new file, count stays at kMaxLogFiles.
    while (files.size() >= Config::MAX_LOG_FILES) {
        QFile::remove(files.first().absoluteFilePath());
        files.removeFirst();
    }
}
QString LogFile::sanitizeFileName(const QString &name){
    // Windows forbids: \ / : * ? " < > | and control characters.
    // Linux only forbids / (and NUL), so covering Windows' rules is enough for both.
    QString sanitized = name;
    sanitized.replace(QRegularExpression(R"([\\/:*?"<>|\x00-\x1F])"), "_");

    // Windows also disallows trailing dots/spaces in a file name.
    while (sanitized.endsWith('.') || sanitized.endsWith(' '))
        sanitized.chop(1);

    if (sanitized.isEmpty())
        sanitized = "log";

    return sanitized;
}
