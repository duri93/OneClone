#include "UpdateManager.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QCryptographicHash>
#include <QProcess>
#include <QRegularExpression>
#include <QUuid>
#include <QUrl>

UpdateManager::UpdateManager(QString repoOwner,
                         QString repoName,
                         QString currentVersion,
                         QObject *parent)
    : QObject(parent)
    , m_owner(std::move(repoOwner))
    , m_repo(std::move(repoName))
    , m_currentVersion(std::move(currentVersion))
    , m_installDir(QCoreApplication::applicationDirPath()){
#if defined(Q_OS_WIN)
    m_assetNameFilter = QStringLiteral("win");
#elif defined(Q_OS_LINUX)
    m_assetNameFilter = QStringLiteral("linux");
#endif
}

UpdateManager::~UpdateManager(){
    if (m_currentReply) {
        m_currentReply->abort();
    }
    if (m_downloadFile.isOpen()) {
        m_downloadFile.close();
    }
    // If the update was armed for install, autoRemove was already disabled
    // and the temp dir is left in place for the install script. Otherwise
    // this wipes it.
    delete m_tempDir;
    m_tempDir = nullptr;
}

bool UpdateManager::isUpdateInProgress() const{
    return !m_currentReply.isNull();
}

bool UpdateManager::isUpdateReady() const{
    return m_updateReady;
}

void UpdateManager::checkForUpdates(){
    if (m_currentReply) {
        return; // a check/download is already running
    }
    m_attemptsUsed = 0;
    m_updateReady = false;
    m_expectedSha256.clear();
    m_checksumUrl.clear();
    emit checkStarted();
    fetchReleaseInfo();
}

void UpdateManager::fetchReleaseInfo(){
    const QString path = QStringLiteral("/repos/%1/%2/releases/latest").arg(m_owner, m_repo);

    QNetworkRequest request{QUrl(QStringLiteral("https://api.github.com") + path)};
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setRawHeader("User-Agent", QStringLiteral("%1-GitHubUpdater").arg(m_repo).toUtf8());
    // Follow redirects automatically (safe policy: won't silently downgrade https->http).
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QVariant::fromValue(QNetworkRequest::NoLessSafeRedirectPolicy));

    m_currentReply = m_netManager.get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &UpdateManager::onReleaseInfoReceived);
}

void UpdateManager::onReleaseInfoReceived(){
    QNetworkReply *reply = m_currentReply;
    if (!reply) {
        return;
    }
    reply->deleteLater();
    m_currentReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        handleFatalFailure(tr("Failed to check for updates: %1").arg(reply->errorString()));
        return;
    }

    const QByteArray body = reply->readAll();
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        handleFatalFailure(tr("Could not parse release information: %1").arg(parseError.errorString()));
        return;
    }

    QJsonObject releaseObj;
    if (m_includePreReleases) {
        const QJsonArray arr = doc.array();
        for (const QJsonValue &v : arr) {
            const QJsonObject o = v.toObject();
            if (!o.value(QStringLiteral("draft")).toBool(false)) {
                releaseObj = o;
                break;
            }
        }
        if (releaseObj.isEmpty()) {
            handleFatalFailure(tr("No published releases found."));
            return;
        }
    } else {
        releaseObj = doc.object();
    }

    const QString tag = releaseObj.value(QStringLiteral("tag_name")).toString();
    if (tag.isEmpty()) {
        handleFatalFailure(tr("Release information did not contain a version tag."));
        return;
    }

    if (compareVersions(tag, m_currentVersion) <= 0) {
        emit noUpdateAvailable(m_currentVersion);
        return;
    }

    ReleaseAsset chosen;
    QString checksumUrl;
    const QJsonArray assets = releaseObj.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue &v : assets) {
        const QJsonObject a = v.toObject();
        const QString name = a.value(QStringLiteral("name")).toString();
        const QString url = a.value(QStringLiteral("browser_download_url")).toString();
        if (name.isEmpty() || url.isEmpty()) {
            continue;
        }
        const QString lower = name.toLower();

        if (chosen.name.isEmpty() &&
            (m_assetNameFilter.isEmpty() || lower.contains(m_assetNameFilter.toLower()))) {
            chosen.name = name;
            chosen.downloadUrl = url;
            chosen.size = a.value(QStringLiteral("size")).toVariant().toLongLong();
        }
        if (checksumUrl.isEmpty() &&
            (lower.contains(QStringLiteral("sha256")) || lower.contains(QStringLiteral("checksum")))) {
            checksumUrl = url;
        }
    }

    if (chosen.name.isEmpty()) {
        handleFatalFailure(tr("Release %1 has no asset matching \"%2\".").arg(tag, m_assetNameFilter));
        return;
    }

    if (!isDirectoryWritable(m_installDir)) {
        emit insufficientPermissions(m_installDir);
        handleFatalFailure(tr("Install directory is not writable: %1").arg(m_installDir));
        return;
    }

    m_selectedAsset = chosen;
    m_checksumUrl = checksumUrl;
    m_newVersion = tag;

    delete m_tempDir;
    m_tempDir = new QTemporaryDir(QDir::tempPath() + QLatin1Char('/') + m_repo + QStringLiteral("-update-XXXXXX"));
    if (!m_tempDir->isValid()) {
        const QString err = m_tempDir->errorString();
        delete m_tempDir;
        m_tempDir = nullptr;
        handleFatalFailure(tr("Could not create a temporary directory: %1").arg(err));
        return;
    }

    emit updateAvailable(m_newVersion);
    startAssetDownload();
}

void UpdateManager::startAssetDownload(){
    ++m_attemptsUsed;
    if (m_attemptsUsed > m_maxAttempts) {
        handleFatalFailure(tr("Download failed after %1 attempt(s).").arg(m_maxAttempts));
        return;
    }

    m_downloadedFilePath = m_tempDir->filePath(m_selectedAsset.name);
    if (m_downloadFile.isOpen()) {
        m_downloadFile.close();
    }
    m_downloadFile.setFileName(m_downloadedFilePath);
    if (!m_downloadFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        handleFatalFailure(tr("Could not write to temporary file: %1").arg(m_downloadFile.errorString()));
        return;
    }

    QNetworkRequest request{QUrl(m_selectedAsset.downloadUrl)};
    request.setRawHeader("Accept", "application/octet-stream");
    request.setRawHeader("User-Agent", QStringLiteral("%1-GitHubUpdater").arg(m_repo).toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QVariant::fromValue(QNetworkRequest::NoLessSafeRedirectPolicy));

    m_currentReply = m_netManager.get(request);
    connect(m_currentReply, &QNetworkReply::readyRead, this, &UpdateManager::onAssetDownloadReadyRead);
    connect(m_currentReply, &QNetworkReply::downloadProgress, this, &UpdateManager::downloadProgress);
    connect(m_currentReply, &QNetworkReply::finished, this, &UpdateManager::onAssetDownloadFinished);
}

void UpdateManager::onAssetDownloadReadyRead(){
    if (m_currentReply && m_downloadFile.isOpen()) {
        m_downloadFile.write(m_currentReply->readAll());
    }
}

void UpdateManager::onAssetDownloadFinished(){
    QNetworkReply *reply = m_currentReply;
    if (!reply) {
        return;
    }

    if (m_downloadFile.isOpen()) {
        m_downloadFile.write(reply->readAll());
        m_downloadFile.close();
    }

    const QNetworkReply::NetworkError err = reply->error();
    const QString errString = reply->errorString();
    reply->deleteLater();
    m_currentReply = nullptr;

    if (err != QNetworkReply::NoError) {
        retryOrFail(tr("Download error: %1").arg(errString));
        return;
    }

    QString verifyError;
    if (!verifySizeOnDisk(&verifyError)) {
        retryOrFail(verifyError);
        return;
    }

    if (!m_checksumUrl.isEmpty()) {
        startChecksumDownload();
    } else {
        finalizeVerification();
    }
}

void UpdateManager::startChecksumDownload(){
    QNetworkRequest request{QUrl(m_checksumUrl)};
    request.setRawHeader("User-Agent", QStringLiteral("%1-GitHubUpdater").arg(m_repo).toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QVariant::fromValue(QNetworkRequest::NoLessSafeRedirectPolicy));
    m_currentReply = m_netManager.get(request);
    connect(m_currentReply, &QNetworkReply::finished, this, &UpdateManager::onChecksumDownloadFinished);
}

void UpdateManager::onChecksumDownloadFinished(){
    QNetworkReply *reply = m_currentReply;
    if (!reply) {
        return;
    }
    reply->deleteLater();
    m_currentReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        // Non-fatal: the size check already passed, proceed without the
        // stronger checksum guarantee.
        finalizeVerification();
        return;
    }

    const QString content = QString::fromUtf8(reply->readAll());
    static const QRegularExpression hashRe(QStringLiteral("\\b[0-9a-fA-F]{64}\\b"));
    static const QRegularExpression lineSplitRe(QStringLiteral("[\\r\\n]+"));

    m_expectedSha256.clear();
    const QStringList lines = content.split(lineSplitRe, Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (lines.size() > 1 && !line.contains(m_selectedAsset.name, Qt::CaseInsensitive)) {
            continue; // multi-entry checksum file: only the line for our asset matters
        }
        const QRegularExpressionMatch m = hashRe.match(line);
        if (m.hasMatch()) {
            m_expectedSha256 = m.captured(0).toLower();
            break;
        }
    }

    if (m_expectedSha256.isEmpty()) {
        // Could not parse a usable hash; fall back to the size check.
        finalizeVerification();
        return;
    }

    QString verifyError;
    if (!verifyChecksum(&verifyError)) {
        retryOrFail(verifyError);
        return;
    }

    finalizeVerification();
}

bool UpdateManager::verifySizeOnDisk(QString *errorOut) const{
    const QFileInfo info(m_downloadedFilePath);
    if (!info.exists() || info.size() == 0) {
        if (errorOut) {
            *errorOut = tr("Downloaded file is missing or empty.");
        }
        return false;
    }
    if (m_selectedAsset.size > 0 && info.size() != m_selectedAsset.size) {
        if (errorOut) {
            *errorOut = tr("Downloaded file size (%1) does not match expected size (%2).")
            .arg(info.size())
                .arg(m_selectedAsset.size);
        }
        return false;
    }
    return true;
}

bool UpdateManager::verifyChecksum(QString *errorOut) const{
    QFile f(m_downloadedFilePath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errorOut) {
            *errorOut = tr("Could not open downloaded file for checksum verification.");
        }
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&f)) {
        if (errorOut) {
            *errorOut = tr("Failed reading downloaded file for checksum verification.");
        }
        return false;
    }

    const QString actual = QString::fromLatin1(hash.result().toHex());
    if (actual.compare(m_expectedSha256, Qt::CaseInsensitive) != 0) {
        if (errorOut) {
            *errorOut = tr("Checksum mismatch: expected %1, got %2.").arg(m_expectedSha256, actual);
        }
        return false;
    }
    return true;
}

void UpdateManager::retryOrFail(const QString &reason){
    QFile::remove(m_downloadedFilePath);
    if (m_attemptsUsed < m_maxAttempts) {
        startAssetDownload();
    } else {
        handleFatalFailure(reason);
    }
}

void UpdateManager::finalizeVerification(){
    m_updateReady = true;
    m_tempDir->setAutoRemove(false); // the install script now owns cleanup of this directory
    armInstallOnQuit();
    emit updateReady(m_newVersion);
}

void UpdateManager::armInstallOnQuit(){
    if (m_installArmed) {
        return;
    }
    m_installArmed = true;
    if (QCoreApplication *app = QCoreApplication::instance()) {
        connect(app, &QCoreApplication::aboutToQuit, this, &UpdateManager::installUpdate);
    }
}

bool UpdateManager::applyUpdateAndRestart(){
    if (!m_updateReady) {
        return false;
    }
    QCoreApplication::quit(); // triggers aboutToQuit -> installUpdate()
    return true;
}

void UpdateManager::handleFatalFailure(const QString &reason){
    cleanupTempDir();
    emit updateFailed(reason);
}

void UpdateManager::cleanupTempDir(){
    if (m_downloadFile.isOpen()) {
        m_downloadFile.close();
    }
    delete m_tempDir; // autoRemove is still true here (finalizeVerification not reached) -> wipes the directory
    m_tempDir = nullptr;
    m_downloadedFilePath.clear();
}

bool UpdateManager::isDirectoryWritable(const QString &path){
    QDir dir(path);
    if (!dir.exists()) {
        return false;
    }
    QTemporaryFile probe(dir.filePath(QStringLiteral(".update_write_test_XXXXXX")));
    return probe.open(); // QTemporaryFile removes itself when it goes out of scope
}

QString UpdateManager::normalizeVersion(const QString &v){
    QString s = v.trimmed();
    if (s.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        s.remove(0, 1);
    }
    return s;
}

int UpdateManager::compareVersions(const QString &a, const QString &b){
    const QStringList pa = normalizeVersion(a).split(QLatin1Char('.'));
    const QStringList pb = normalizeVersion(b).split(QLatin1Char('.'));
    const int n = qMax(pa.size(), pb.size());
    for (int i = 0; i < n; ++i) {
        const QString sa = i < pa.size() ? pa.at(i) : QStringLiteral("0");
        const QString sb = i < pb.size() ? pb.at(i) : QStringLiteral("0");

        bool okA = false;
        bool okB = false;
        const qlonglong na = sa.toLongLong(&okA);
        const qlonglong nb = sb.toLongLong(&okB);

        if (okA && okB) {
            if (na != nb) {
                return na < nb ? -1 : 1;
            }
        } else {
            const int c = sa.compare(sb);
            if (c != 0) {
                return c < 0 ? -1 : 1;
            }
        }
    }
    return 0;
}

QString UpdateManager::escapeForBatch(const QString &s){
    // Used inside `set "VAR=value"` assignments. Double quotes are not
    // valid in Windows paths, so the only character that needs escaping in
    // that context is a literal '%' (batch variable-expansion character).
    QString out = s;
    out.replace(QLatin1Char('%'), QStringLiteral("%%"));
    return out;
}

QString UpdateManager::quoteForShell(const QString &s){
    // POSIX single-quote escaping: close quote, insert escaped quote, reopen.
    QString out = s;
    out.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + out + QLatin1Char('\'');
}

QString UpdateManager::writeWindowsScript(qint64 pid) const{
    const QString scriptPath = QDir::tempPath() + QLatin1String("/ghupdater_apply_")
    + QUuid::createUuid().toString(QUuid::WithoutBraces) + QLatin1String(".bat");

    const QString archive = QDir::toNativeSeparators(m_downloadedFilePath);
    const QString extractDir = QDir::toNativeSeparators(m_tempDir->filePath(QStringLiteral("extracted")));
    const QString tempDirPath = QDir::toNativeSeparators(m_tempDir->path());
    const QString installDir = QDir::toNativeSeparators(m_installDir);
    const QString appExe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

    const QString script = QStringLiteral(
                               "@echo off\r\n"
                               "setlocal\r\n"
                               "set \"PID=%1\"\r\n"
                               "set \"ARCHIVE=%2\"\r\n"
                               "set \"EXTRACT_DIR=%3\"\r\n"
                               "set \"INSTALL_DIR=%4\"\r\n"
                               "set \"APP_EXE=%5\"\r\n"
                               "set \"TEMP_DIR=%6\"\r\n"
                               "\r\n"
                               ":waitloop\r\n"
                               "tasklist /FI \"PID eq %PID%\" 2>NUL | find \"%PID%\" >NUL\r\n"
                               "if not errorlevel 1 (\r\n"
                               "    timeout /t 1 /nobreak >NUL\r\n"
                               "    goto waitloop\r\n"
                               ")\r\n"
                               "\r\n"
                               "mkdir \"%EXTRACT_DIR%\" >NUL 2>NUL\r\n"
                               "tar -xf \"%ARCHIVE%\" -C \"%EXTRACT_DIR%\"\r\n"
                               "if errorlevel 1 goto cleanup\r\n"
                               "\r\n"
                               "robocopy \"%EXTRACT_DIR%\" \"%INSTALL_DIR%\" /E /IS /IT /NFL /NDL /NJH /NJS\r\n"
                               "if errorlevel 8 goto cleanup\r\n"
                               "\r\n"
                               "start \"\" \"%APP_EXE%\"\r\n"
                               "\r\n"
                               ":cleanup\r\n"
                               "rd /s /q \"%TEMP_DIR%\" >NUL 2>NUL\r\n"
                               "del \"%~f0\" >NUL 2>NUL\r\n"
                               ).arg(QString::number(pid),
                                    escapeForBatch(archive),
                                    escapeForBatch(extractDir),
                                    escapeForBatch(installDir),
                                    escapeForBatch(appExe),
                                    escapeForBatch(tempDirPath));

    QFile f(scriptPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return QString();
    }
    f.write(script.toUtf8());
    f.close();
    return scriptPath;
}

QString UpdateManager::writeLinuxScript(qint64 pid) const{
    const QString scriptPath = QDir::tempPath() + QLatin1String("/ghupdater_apply_")
    + QUuid::createUuid().toString(QUuid::WithoutBraces) + QLatin1String(".sh");

    const QString archive = m_downloadedFilePath;
    const QString extractDir = m_tempDir->filePath(QStringLiteral("extracted"));
    const QString tempDirPath = m_tempDir->path();
    const QString installDir = m_installDir;
    const QString appExe = QCoreApplication::applicationFilePath();

    const QString script = QStringLiteral(
                               "#!/bin/sh\n"
                               "PID=%1\n"
                               "ARCHIVE=%2\n"
                               "EXTRACT_DIR=%3\n"
                               "INSTALL_DIR=%4\n"
                               "APP_EXE=%5\n"
                               "TEMP_DIR=%6\n"
                               "\n"
                               "trap 'rm -rf \"$TEMP_DIR\"; rm -f -- \"$0\"' EXIT\n"
                               "\n"
                               "while kill -0 \"$PID\" 2>/dev/null; do\n"
                               "    sleep 0.5\n"
                               "done\n"
                               "\n"
                               "mkdir -p \"$EXTRACT_DIR\" || exit 1\n"
                               "tar -xzf \"$ARCHIVE\" -C \"$EXTRACT_DIR\" || exit 1\n"
                               "cp -rf \"$EXTRACT_DIR\"/. \"$INSTALL_DIR\"/ || exit 1\n"
                               "chmod +x \"$APP_EXE\" 2>/dev/null\n"
                               "\n"
                               "nohup \"$APP_EXE\" >/dev/null 2>&1 &\n"
                               ).arg(QString::number(pid),
                                    quoteForShell(archive),
                                    quoteForShell(extractDir),
                                    quoteForShell(installDir),
                                    quoteForShell(appExe),
                                    quoteForShell(tempDirPath));

    QFile f(scriptPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return QString();
    }
    f.write(script.toUtf8());
    f.close();
    f.setPermissions(f.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeUser | QFileDevice::ExeGroup);
    return scriptPath;
}

void UpdateManager::installUpdate(){
    if (!m_updateReady || !m_tempDir) {
        return;
    }

    const qint64 pid = QCoreApplication::applicationPid();

#if defined(Q_OS_WIN)
    const QString scriptPath = writeWindowsScript(pid);
    if (scriptPath.isEmpty()) {
        return;
    }
    // Argument-list form: no shell is invoked to parse this command, so
    // nothing in scriptPath (or the paths embedded inside it) can be
    // interpreted as shell syntax here.
    QProcess::startDetached(QStringLiteral("cmd.exe"),
                            {QStringLiteral("/C"), QDir::toNativeSeparators(scriptPath)});
#elif defined(Q_OS_LINUX)
    const QString scriptPath = writeLinuxScript(pid);
    if (scriptPath.isEmpty()) {
        return;
    }
    QProcess::startDetached(QStringLiteral("/bin/sh"), {scriptPath});
#endif
}