// RemotesList.h

#pragma once

#include <QCompleter>
#include <QLineEdit>
#include <QStringList>

class RemotesAutocompleter
{
public:
    // Creates the completer, populates it from rclone, and applies it to the line edit.
    // Returns nullptr if the rclone lookup fails.
    static QCompleter *setup(QLineEdit *lineEdit, const QString &rclonePath);

    static QStringList listRemotes(const QString &rclonePath);
private:
    static QStringList listDirs(const QString &rclonePath, const QString &remote);
    static QStringList buildEntries(const QString &rclonePath);

    static bool runRclone(const QString &rclonePath,
                          const QStringList &arguments,
                          QString *stdoutText);
};
