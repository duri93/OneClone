# Release Publishing Guide

## Step-by-step

1. **Bump the version**
   - Update `inline constexpr char APP_VERSION[] = "1.4";` in `model/Config.h` to the new version (e.g. `"1.5"`).
   - Commit this change to the repository.

2. **Push and tag the release commit**
   - Commit all changes (iteally only the version bump)
   - Tag the commit with the release version (e.g. `"v1.5"`)
   - Push to origin

Or via command line
```bash
   git add model/Config.h
   git commit -m "Bump version to 1.5"
   git push origin main
   git tag v1.5
   git push origin v1.5
```

3. **Build the Windows release**
  - On a windows machine with proper build setup, run `release/win-build-deploy.ps1` in powershell
  - Add the version number to the newly created `release/OneClone-win.zip` (e.g. `OneClone-1.5-win.zip`). 
      Note:
      - the zip directly contains the app files, with no wrapper directory.
      - the zip name must contain `win`
      - hash file is optional but strongly recommended

Or via command line
```bash
   cmake -S . -B build-win -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64"
   cmake --build build-win --config Release
   windeployqt --release --no-translations build-win/OneClone.exe
   sha256sum OneClone-1.5-win.zip > OneClone-1.5-win.sha256.txt
```

4. **Build the linux release**
Procedure will be implemented once linux is supported.
<!--
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
-->

5. **Create the GitHub Release**
Via the GitHub web UI: 
- **Releases → Draft a new release**
- choose tag `v1.5`
- upload the files (windows and linux zip and sha256)
- write release notes
- **Publish release**

Note: pre-releases will not trigger auto-update.

Alternatively:
```bash
     gh release create v1.5 \
       dist/OneClone-1.5-win.zip \
       dist/OneClone-1.5-win.sha256.txt \
       dist/OneClone-1.5-linux.tar.gz \
       dist/OneClone-1.5-linux.sha256.txt \
       --title "v1.5" \
       --notes "Release notes here"
```
