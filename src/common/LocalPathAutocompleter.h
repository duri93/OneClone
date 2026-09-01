/*
// A folder-picker field (e.g. "where should backups go")
LocalPathAutocompleter::attach(destinationLineEdit);

// The rclone.exe field: navigate folders as normal, but only rclone.exe
// itself ever shows up as a completable file
LocalPathAutocompleter::attach(rclonePathLineEdit,
                                LocalPathAutocompleter::Mode::FoldersAndFiles,
                                {"rclone.exe"});

// Or any executable, if you don't want to hardcode the name:
LocalPathAutocompleter::attach(rclonePathLineEdit,
                                LocalPathAutocompleter::Mode::FoldersAndFiles,
                                {"*.exe"});
LocalPathAutocompleter::applyUsageHint(pathLineEdit);
// sets placeholder text + tooltip to:
// "Enter a local path (e.g. D:\Backups) or a remote one (e.g. myremote:myfolder)"
*/
#pragma once

#include <QCompleter>
#include <QLineEdit>
#include <QObject>
#include <QStringList>

class QFileSystemModel;

// Attaches a QCompleter to a QLineEdit that autocompletes local Windows
// paths (drive letters, then one path segment at a time - e.g.
// "C:\Us" -> "C:\Users" -> "C:\Users\Public" ...).
//
// Backed by QFileSystemModel, which lists each directory's contents lazily
// and on a background thread as the user navigates deeper, so typing never
// blocks the UI even on slow (e.g. network) drives.
//
// Lifetime: LocalPathAutocompleter::attach() parents the object to the
// QLineEdit, so it is cleaned up automatically when the line edit is
// destroyed.
class LocalPathAutocompleter : public QObject
{
    Q_OBJECT

public:
    enum class Mode {
        FoldersOnly,     // only directories are offered as completions (default)
        FoldersAndFiles, // directories are always offered; matching files are too
    };

    // fileNameFilters is only used when mode is FoldersAndFiles. It follows
    // QDir/QFileSystemModel wildcard syntax, e.g. {"rclone.exe"} to only
    // ever surface that one file, or {"*.exe"} for any executable. Leave it
    // empty to show every file. Folders are always shown regardless of
    // fileNameFilters, so the user can keep navigating down to find the
    // file they want.
    static LocalPathAutocompleter *attach(QLineEdit *lineEdit,
                                          Mode mode = Mode::FoldersOnly,
                                          const QStringList &fileNameFilters = {});

    explicit LocalPathAutocompleter(QLineEdit *lineEdit,
                                    Mode mode,
                                    const QStringList &fileNameFilters = {},
                                    QObject *parent = nullptr);

    QCompleter *completer() const { return m_completer; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onCompletionActivated(const QString &text);

private:
    QFileSystemModel *m_model = nullptr;
    QCompleter *m_completer = nullptr;
    QLineEdit* m_lineEdit = nullptr;
};