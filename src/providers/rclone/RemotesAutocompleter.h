#pragma once

#include "src/providers/rclone/RCloneProvider.h"

#include <QObject>
#include <QPointer>

class AsyncRunner;
class QCompleter;
class QLineEdit;
class QStringListModel;

// ---------------------------------------------------------------------------
// RemotesAutocompleter
// Attaches a QCompleter to a QLineEdit and populates it asynchronously from
// `rclone listremotes` / `rclone lsd`, so the UI thread never blocks.
// Entries appear incrementally as each remote/directory listing completes.
//
// Lifetime: RemotesAutocompleter::attach() parents the object (and, via it,
// its AsyncRunner/background thread) to the QLineEdit, so everything is
// cleaned up automatically when the line edit is destroyed.
// ---------------------------------------------------------------------------
class RemotesAutocompleter : public QObject
{
    Q_OBJECT

public:
    // Convenience factory: creates the autocompleter, attaches its completer
    // to lineEdit, and starts the async lookup. Returns nullptr if lineEdit
    // is null. The returned object is parented to lineEdit.
    static RemotesAutocompleter *attach(QLineEdit *lineEdit, RCloneProvider *rcloneProvider, const QString &rclonePath);

    explicit RemotesAutocompleter(QLineEdit *lineEdit, RCloneProvider *rcloneProvider, QString rclonePath, QObject *parent = nullptr);
    ~RemotesAutocompleter() override = default;

    QCompleter *completer() const { return m_completer; }

signals:
    // Emitted once the whole lookup (remote list + every directory listing)
    // has finished, regardless of whether anything was found.
    void lookupFinished();

private slots:
    void onRemotesReady(const QStringList &remotes);
    void onDirsReady(const QString &remote, const QStringList &dirs);

private:
    void addEntries(const QStringList &newEntries);

    QPointer<QLineEdit> m_lineEdit;
    QCompleter *m_completer = nullptr;
    QStringListModel *m_model = nullptr;
    QStringList m_entries;

    // Owns the background thread the lookup runs on; parented to `this`,
    // so it's cleaned up (and the thread stopped) automatically when this
    // RemotesAutocompleter is destroyed — see AsyncRunner.
    AsyncRunner *m_runner = nullptr;
};

