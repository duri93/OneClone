#include "RemotesAutocompleter.h"

#include "src/common/AsyncRunner.h"
#include "src/providers/rclone/RemotesLookupWorker.h"

#include <QCompleter>
#include <QLineEdit>
#include <QStringListModel>

// ---------------------------------------------------------------------------
// RemotesAutocompleter
// ---------------------------------------------------------------------------

RemotesAutocompleter *RemotesAutocompleter::attach(QLineEdit *lineEdit, RCloneProvider *rcloneProvider, const QString &rclonePath)
{
    if (!lineEdit)
        return nullptr;

    // Parented to lineEdit: destroyed automatically along with it.
    return new RemotesAutocompleter(lineEdit, rcloneProvider, rclonePath, lineEdit);
}

RemotesAutocompleter::RemotesAutocompleter(QLineEdit *lineEdit, RCloneProvider *rcloneProvider, QString rclonePath, QObject *parent)
    : QObject(parent)
    , m_lineEdit(lineEdit)
{
    // Wire up the completer immediately, with an empty model, so the line
    // edit is fully usable from the start. Entries get added to m_model as
    // the background lookup reports them in.
    m_model = new QStringListModel(this);

    m_completer = new QCompleter(this);
    m_completer->setModel(m_model);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchStartsWith);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);

    if (lineEdit)
        lineEdit->setCompleter(m_completer);

    auto *worker = new RemotesLookupWorker(rcloneProvider, std::move(rclonePath));

    // AsyncRunner moves `worker` onto a background thread and starts that
    // thread immediately. This is a one-shot job: run it as soon as the
    // thread starts, and let the thread (and worker) stop themselves once
    // finished() fires, rather than waiting for this RemotesAutocompleter
    // to be destroyed.
    m_runner = new AsyncRunner(worker, this);
    connect(&m_runner->backgroundThread(), &QThread::started, worker, &RemotesLookupWorker::run);
    connect(worker, &RemotesLookupWorker::finished, &m_runner->backgroundThread(), &QThread::quit);

    connect(worker, &RemotesLookupWorker::remotesReady, this, &RemotesAutocompleter::onRemotesReady);
    connect(worker, &RemotesLookupWorker::dirsReady, this, &RemotesAutocompleter::onDirsReady);
    connect(worker, &RemotesLookupWorker::finished, this, &RemotesAutocompleter::lookupFinished);
}

void RemotesAutocompleter::onRemotesReady(const QStringList &remotes)
{
    QStringList entries;
    entries.reserve(remotes.size());

    for (const QString &remote : remotes)
        entries.append(remote + ":");

    addEntries(entries);
}

void RemotesAutocompleter::onDirsReady(const QString &remote, const QStringList &dirs)
{
    QStringList entries;
    entries.reserve(dirs.size());

    for (const QString &dir : dirs)
        entries.append(remote + ":" + dir);

    addEntries(entries);
}

void RemotesAutocompleter::addEntries(const QStringList &newEntries)
{
    if (newEntries.isEmpty())
        return;

    m_entries.append(newEntries);
    m_entries.removeDuplicates();
    m_entries.sort(Qt::CaseInsensitive);

    m_model->setStringList(m_entries);

    if (m_lineEdit && m_lineEdit->hasFocus()) {
        m_completer->setCompletionPrefix(m_lineEdit->text());
        m_completer->complete();
    }
}
