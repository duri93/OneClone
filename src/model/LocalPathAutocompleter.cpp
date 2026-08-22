// LocalPathAutocompleter.cpp

#include "LocalPathAutocompleter.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QKeyEvent>


namespace {

// QFileSystemModel's default icon provider asks the OS shell for a "proper"
// icon for every row it lists, which can be noticeably slow on network
// drives or folders with many entries. We don't show icons in a completer
// popup anyway, so skip that work entirely.
class NoIconProvider : public QFileIconProvider
{
public:
    QIcon icon(const QFileInfo &) const override { return QIcon(); }
};

} // namespace

LocalPathAutocompleter *LocalPathAutocompleter::attach(QLineEdit *lineEdit,
                                                       Mode mode,
                                                       const QStringList &fileNameFilters)
{
    if (!lineEdit)
        return nullptr;

    // Parented to lineEdit: destroyed automatically along with it.
    return new LocalPathAutocompleter(lineEdit, mode, fileNameFilters, lineEdit);
}

LocalPathAutocompleter::LocalPathAutocompleter(QLineEdit *lineEdit,
                                               Mode mode,
                                               const QStringList &fileNameFilters,
                                               QObject *parent)
    : QObject(parent), m_lineEdit(lineEdit)
{
    m_model = new QFileSystemModel(this);
    m_model->setIconProvider(new NoIconProvider);

    // Empty root path -> the model covers the whole tree (drives at the top
    // level on Windows), which is what lets the completer resolve "C:\...",
    // "D:\...", etc. Each subdirectory's contents are then fetched lazily,
    // in the background, the first time the completer needs them.
    m_model->setRootPath(QString());

    // QDir::AllDirs makes directories bypass name filters entirely, so
    // folders always stay visible/navigable even when fileNameFilters
    // narrows down which *files* show up (e.g. only "rclone.exe").
    QDir::Filters filters = QDir::NoDotAndDotDot | QDir::Drives | QDir::AllDirs;

    if (mode == Mode::FoldersAndFiles) {
        filters |= QDir::Files;

        if (!fileNameFilters.isEmpty()) {
            m_model->setNameFilters(fileNameFilters);
            // false = entries that don't match are hidden outright, rather
            // than shown grayed-out/disabled. We want a clean list of
            // real completions, not a browse-everything view.
            m_model->setNameFilterDisables(false);
        }
    }

    m_model->setFilter(filters);

    m_completer = new QCompleter(this);
    m_completer->setModel(m_model);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    // QCompleter has built-in special-case handling for QFileSystemModel:
    // it already knows how to split "C:\Users\Pub" into drive + path
    // segments and complete one segment at a time, so no further
    // configuration (splitPath/pathFromIndex overrides) is needed here.

    lineEdit->setCompleter(m_completer);
    lineEdit->installEventFilter(this);

    connect(m_completer, QOverload<const QString &>::of(&QCompleter::activated),
            this, &LocalPathAutocompleter::onCompletionActivated);
}

bool LocalPathAutocompleter::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_lineEdit && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);

        if (keyEvent->key() == Qt::Key_Tab && m_completer->popup()->isVisible()) {
            QAbstractItemView *popup = m_completer->popup();

            QModelIndex index = popup->currentIndex();
            if (!index.isValid())
                index = m_completer->completionModel()->index(0, m_completer->completionColumn());

            if (index.isValid()) {
                popup->setCurrentIndex(index);

                // Forward a synthetic Enter to the popup: QCompleter installs
                // its own event filter there that treats Enter as "accept the
                // highlighted row", so this reuses its exact logic (including
                // emitting activated()) instead of duplicating it here.
                QKeyEvent enterEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
                QCoreApplication::sendEvent(popup, &enterEvent);
            }

            return true; // swallow Tab so focus doesn't move to the next widget
        }
    }

    return QObject::eventFilter(watched, event);
}

void LocalPathAutocompleter::onCompletionActivated(const QString &text)
{
    if (!m_lineEdit || !QFileInfo(text).isDir())
        return;

    QString path = text;
    if (!path.endsWith(QLatin1Char('\\')) && !path.endsWith(QLatin1Char('/')))
        path += QLatin1Char('\\');

    m_lineEdit->setText(path);
    m_lineEdit->setCursorPosition(path.length());

    // Re-show the popup immediately with this folder's contents. setText()
    // alone won't do this: QCompleter only auto-updates/pops up in response
    // to the user typing (textEdited), not to a programmatic setText().
    m_completer->setCompletionPrefix(path);
    m_completer->complete();
}


