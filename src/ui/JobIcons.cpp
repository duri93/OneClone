#include "JobIcons.h"

#include <QHash>
#include <QPainter>
#include <QSvgRenderer>

namespace {

QString statusIconName(JobStatus status)
{
    switch (status) {
    case JobStatus::Stopped:  return QStringLiteral("stopped");
    case JobStatus::Starting: return QStringLiteral("starting");
    case JobStatus::Running:  return QStringLiteral("running");
    case JobStatus::Success:  return QStringLiteral("success");
    case JobStatus::Stopping: return QStringLiteral("stopping");
    case JobStatus::Errored:  return QStringLiteral("errored");
    }
    return QStringLiteral("stopped"); // unreachable, keeps compilers happy
}

QPixmap renderJobIcon(JobType type, bool active, int size)
{
    QString path = QStringLiteral(":/icons/%1_%2.svg")
        .arg(jobTypeToString(type), active ? QStringLiteral("active") : QStringLiteral("inactive"));

    return QPixmap(path).scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap renderStatusIcon(JobStatus status, bool hasWarning, int size)
{
    QString basePath = QStringLiteral(":/icons/%1.svg").arg(statusIconName(status));

    QPixmap pixmap(basePath);
    pixmap = pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    if (hasWarning) {
        QSvgRenderer warnRenderer(QStringLiteral(":/icons/warning.svg"));

        int warnSize = size / 3;
        QRect warnRect(size - warnSize, size - warnSize, warnSize, warnSize);

        QPainter painter(&pixmap);
        warnRenderer.render(&painter, warnRect);
    }

    return pixmap;
}

} // namespace

QPixmap JobIcons::jobIcon(JobType type, bool active, int size)
{
    static QHash<QString, QPixmap> cache;

    QString key = QStringLiteral("%1|%2|%3")
        .arg(jobTypeToString(type)).arg(active).arg(size);

    auto it = cache.constFind(key);
    if (it != cache.constEnd()) return it.value();

    return cache.insert(key, renderJobIcon(type, active, size)).value();
}

QPixmap JobIcons::statusIcon(JobStatus status, bool hasWarning, int size)
{
    static QHash<QString, QPixmap> cache;

    QString key = QStringLiteral("%1|%2|%3")
        .arg(statusIconName(status)).arg(hasWarning).arg(size);

    auto it = cache.constFind(key);
    if (it != cache.constEnd()) return it.value();

    return cache.insert(key, renderStatusIcon(status, hasWarning, size)).value();
}
