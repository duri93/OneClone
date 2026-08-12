# Release Publishing Guide

This assumes the `UpdateManager` in-app auto-updater's expectations are honored:
- GitHub Releases must be **published** (not Draft, not Pre-release) — the code queries `/repos/{owner}/{repo}/releases/latest`, which GitHub only returns for the newest non-draft, non-prerelease release (the app's `m_includePreReleases` flag defaults to `false` and isn't currently exposed in the UI).
- The release **tag** is compared against `Config::APP_VERSION` via `UpdateManager::normalizeVersion()`, which only strips a single leading `v`/`V`. So if `APP_VERSION` is `"1.5"`, the tag should be `v1.5` (or `1.5`).
- Exactly one asset name must contain **`win`** (Windows build, `.zip`) and one must contain **`linux`** (Linux build, `.tar.gz`), matching `m_assetNameFilter` (`"win"` on `Q_OS_WIN`, `"linux"` on `Q_OS_LINUX`).
- An optional checksum asset with `sha256` or `checksum` in its filename enables strong integrity verification (`verifyChecksum()`); without it, only a size check is performed.

## Step-by-step

1. **Bump the version**
   - Update `inline constexpr char APP_VERSION[] = "1.4";` in `model/Config.h` to the new version (e.g. `"1.5"`).
   - Commit this change to the repository.

2. **Push and tag the release commit**
```bash
   git add model/Config.h
   git commit -m "Bump version to 1.5"
   git push origin main
   git tag v1.5
   git push origin v1.5
```

3. **Build the Windows release**
```bash
   cmake -S . -B build-win -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64"
   cmake --build build-win --config Release
```
   - Run `windeployqt` on the built executable to gather Qt DLLs and plugins into the output folder:
```bash
     windeployqt --release --no-translations build-win/OneClone.exe
```
   - Collect the final folder contents (`OneClone.exe` + deployed DLLs/plugins/resources) into a clean staging directory, e.g. `dist/OneClone-1.5-win/`.

4. **Package the Windows build**
   - Zip the staging folder's *contents* (so the archive extracts directly to the app files, matching what the in-app updater's `writeWindowsScript` extracts and `robocopy`s into the install dir):
```bash
     cd dist/OneClone-1.5-win
     zip -r ../OneClone-1.5-win.zip .
```
   - Name: **`OneClone-1.5-win.zip`** (must contain `win`).
   - Optionally generate a checksum file:
```bash
     sha256sum OneClone-1.5-win.zip > OneClone-1.5-win.sha256.txt
```

5. **Build the Linux release**
```bash
   cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/gcc_64"
   cmake --build build-linux --parallel
   cmake --install build-linux --prefix dist/OneClone-1.5-linux
```
   - The `install(...)` + `qt_generate_deploy_app_script(...)` already configured in `CMakeLists.txt` will invoke Qt's deploy tooling to bundle the required Qt shared libraries alongside the binary during `cmake --install`.

6. **Package the Linux build**
```bash
   cd dist
   tar czf OneClone-1.5-linux.tar.gz OneClone-1.5-linux
   sha256sum OneClone-1.5-linux.tar.gz > OneClone-1.5-linux.sha256.txt
```
   - Name: **`OneClone-1.5-linux.tar.gz`** (must contain `linux`).

7. **Create the GitHub Release**
   - Via `gh` CLI:
```bash
     gh release create v1.5 \
       dist/OneClone-1.5-win.zip \
       dist/OneClone-1.5-win.sha256.txt \
       dist/OneClone-1.5-linux.tar.gz \
       dist/OneClone-1.5-linux.sha256.txt \
       --title "v1.5" \
       --notes "Release notes here"
```
   - Or via the GitHub web UI: **Releases → Draft a new release**, choose tag `v1.5`, upload the four files, write release notes, and **Publish release** (ensure "Set as a pre-release" is **unchecked**, since the app only queries `/releases/latest` which excludes pre-releases and drafts).

8. **Verify the auto-update flow**
   - Launch a previous build of OneClone.
   - Confirm it detects the new release (`UpdateManager::checkForUpdates()` → `updateAvailable` → download → `updateReady`), shows the "Version X will be installed on next restart" banner, and that quitting/restarting the app correctly applies the update (Windows: `.bat` via `robocopy`; Linux: `.sh` via `cp -rf`).