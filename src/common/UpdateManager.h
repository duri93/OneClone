#pragma once

#include <QFile>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>

class QNetworkReply;
class QTemporaryDir;

// ---------------------------------------------------------------------------
// UpdateManager
// Self-contained, asynchronous auto-updater for apps published as GitHub
// Releases: checks for a newer release, downloads and verifies it, then
// applies it automatically the next time the application quits.
// ---------------------------------------------------------------------------
class UpdateManager : public QObject
{
    Q_OBJECT

public:
    // repoOwner/repoName: the GitHub "owner/repo" whose Releases are checked.
    // currentVersion: the running application's version (e.g. "1.4.2" or "v1.4.2").
    explicit UpdateManager(QString repoOwner,
                           QString repoName,
                           QString currentVersion,
                           QObject *parent = nullptr);
    ~UpdateManager() override;

    bool isUpdateInProgress() const;
    bool isUpdateReady() const;

public slots:
    // Starts an asynchronous check. No-op if a check/download is already running.
    void checkForUpdates();

signals:
    void checkStarted();
    void noUpdateAvailable(const QString &currentVersion);
    void updateAvailable(const QString &newVersion);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

    // The new version has been fully downloaded and verified. It will be
    // installed automatically next time the application quits.
    void updateReady(const QString &newVersion);

    void updateFailed(const QString &reason);

    // The configured install directory is not writable by the current
    // user/process. No update will be downloaded or applied. Emitted in
    // addition to (just before) updateFailed().
    void insufficientPermissions(const QString &directory);

private slots:
    void onReleaseInfoReceived();
    void onAssetDownloadReadyRead();
    void onAssetDownloadFinished();
    void installUpdate();

private:
    struct ReleaseAsset {
        QString name;
        QString downloadUrl;
        qint64 size = -1;
    };

    void fetchReleaseInfo();
    void startAssetDownload();
    void finalizeVerification();
    void retryOrFail(const QString &reason);
    void handleFatalFailure(const QString &reason);
    bool verifySizeOnDisk(QString *errorOut) const;
    bool verifyChecksum(QString *errorOut) const;
    void armInstallOnQuit();
    QString writeWindowsScript(qint64 pid) const;
    QString writeLinuxScript(qint64 pid) const;
    static QString escapeForBatch(const QString &s);
    static QString quoteForShell(const QString &s);
    static int compareVersions(const QString &a, const QString &b);
    static QString normalizeVersion(const QString &v);
    static bool isDirectoryWritable(const QString &path);
    void cleanupTempDir();

    QString m_owner;
    QString m_repo;
    QString m_currentVersion;
    QString m_assetNameFilter;
    QString m_installDir;
    bool m_includePreReleases = false;
    int m_maxAttempts = 3;
    int m_attemptsUsed = 0;

    QNetworkAccessManager m_netManager;
    QPointer<QNetworkReply> m_currentReply;
    QFile m_downloadFile;

    ReleaseAsset m_selectedAsset;
    QString m_expectedSha256;

    QTemporaryDir *m_tempDir = nullptr;
    QString m_downloadedFilePath;
    QString m_newVersion;
    bool m_updateReady = false;
    bool m_installArmed = false;
};