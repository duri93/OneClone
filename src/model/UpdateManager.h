#pragma once
#include <QNetworkAccessManager>
#include <QPointer>
#include <QFile>
class QTemporaryDir;

/**
 * GitHubUpdater
 * =============
 * A small, self-contained auto-updater for Qt desktop applications that are
 * published as GitHub Releases.
 *
 * Flow:
 *   1. checkForUpdates() asks the GitHub API for the latest release and
 *      compares its tag against the current version.
 *   2. If newer, the matching release asset is downloaded to a temporary
 *      directory (HTTP redirects are followed automatically).
 *   3. The download is verified (size, and SHA-256 using the "digest"
 *      field GitHub reports for each release asset, when present).
 *      Corrupted downloads are retried up to setMaxDownloadAttempts()
 *      times.
 *   4. Once verified, updateReady() is emitted so the application can tell
 *      the user. The update is applied automatically the next time the
 *      application quits (via QCoreApplication::aboutToQuit), by handing
 *      off to a small, freshly-generated platform-native script that waits
 *      for this process to exit, replaces the installed files, and
 *      restarts the application.
 *
 * Everything is asynchronous and non-blocking. No settings are persisted
 * to disk by this class -- setMaxDownloadAttempts() only affects the
 * current process and always resets to its default on the next launch.
 *
 * Requirements on the release assets (see accompanying guide):
 *   - Windows asset: a .zip archive containing the full application folder.
 *   - Linux asset:   a .tar.gz archive containing the full application folder.
 *   - No separate checksum asset is required: GitHub computes and reports
 *     a SHA-256 "digest" for each uploaded asset via the Releases API,
 *     which is used for strong integrity verification automatically. If
 *     GitHub does not report a digest for a given asset, verification
 *     falls back to the size check alone.
 */
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