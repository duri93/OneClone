<#
    build-deploy.ps1

    Builds a Qt/CMake project (Release), runs windeployqt, zips the deployment
    folder (no wrapping folder inside the zip) and copies it to DEPLOYMENT_FILE.

    EDIT THE VARIABLES BELOW BEFORE RUNNING.
#>

Start-Transcript -Path "C:\logs\log.txt"

$ErrorActionPreference = "Stop"

# ============================================================
# 1) VARIABLES - EDIT THESE
# ============================================================

# --- Project-specific ---
$ProjectName    = "OneClone"
$ProjectFolder  = Join-Path "Z:\" $ProjectName                    # <-- source project folder (contains CMakeLists.txt)
$DeploymentFile = Join-Path $ProjectFolder "release\OneClone-win.zip" # <-- final zip file (full path incl. filename)
$ExeName        = $ProjectName + ".exe"                             # <-- built executable name

# Folders/files to exclude when copying PROJECT_FOLDER -> C:\temp\src
$ExcludeDirs  = @("build", ".git", ".qtcreator", "release")
$ExcludeFiles = @(".gitattributes", ".gitignore")

# --- Working folders ---
$TempRoot       = "C:\temp"
$CompileFolder  = Join-Path $TempRoot "compile"
$SrcFolder      = Join-Path $TempRoot "src"
$BuildDir       = Join-Path $TempRoot "build"
$DeployDir      = Join-Path $TempRoot "deploy"

# --- Fixed tool locations (from your QtCreator setup) ---
$CMakeExe       = "C:\Qt\Tools\CMake_64\bin\cmake.exe"
$QtBinDir       = "C:\Qt\6.11.1\msvc2022_64\bin"
$WindeployqtExe = Join-Path $QtBinDir "windeployqt.exe"
$QMakeExe       = Join-Path $QtBinDir "qmake.exe"
$MsvcCompiler   = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\HostX64\x64\cl.exe"
$VcVarsAllBat = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
$JomDir       = "C:\Qt\Tools\QtCreator\bin\jom"   # folder containing jom.exe

# ============================================================
# Helper: run robocopy and treat exit codes 0-7 as success
# ============================================================
function Invoke-RobocopySafe {
    param(
        [string]$Source,
        [string]$Destination,
        [string[]]$XD = @(),
        [string[]]$XF = @()
    )

    $roboArgs = @($Source, $Destination, "/E", "/NFL", "/NDL", "/NJH")
    if ($XD.Count -gt 0) { $roboArgs += "/XD"; $roboArgs += $XD }
    if ($XF.Count -gt 0) { $roboArgs += "/XF"; $roboArgs += $XF }

    & robocopy @roboArgs | Out-Null
    $code = $LASTEXITCODE

    if ($code -ge 8) {
        throw "Robocopy failed copying '$Source' -> '$Destination' (exit code $code)"
    }
    return $code
}
function Import-VcVarsEnvironment {
    param(
        [string]$VcVarsAllPath,
        [string]$Arch = "x64"
    )

    Write-Host "==> Importing MSVC build environment from $VcVarsAllPath..."

    $cmdOutput = & cmd.exe /c "`"$VcVarsAllPath`" $Arch && set"

    if ($LASTEXITCODE -ne 0) {
        throw "vcvarsall.bat failed to initialize the environment (exit code $LASTEXITCODE)"
    }

    foreach ($line in $cmdOutput) {
        if ($line -match "^([^=]+)=(.*)$") {
            $name  = $matches[1]
            $value = $matches[2]
            Set-Item -Path "Env:\$name" -Value $value
        }
    }
}

try {

    # ============================================================
    # 1) Clean previous run (so build dir doesn't get created accidentally
    #    via a leftover 'build' folder inside the copied source)
    # ============================================================
    Write-Host "==> Cleaning $TempRoot for a fresh run..."
    if (Test-Path $TempRoot) {
        Remove-Item -Path $TempRoot -Recurse -Force
    }
    New-Item -ItemType Directory -Path $SrcFolder     -Force | Out-Null
    New-Item -ItemType Directory -Path $BuildDir      -Force | Out-Null
    New-Item -ItemType Directory -Path $DeployDir     -Force | Out-Null
    New-Item -ItemType Directory -Path $CompileFolder -Force | Out-Null

    # ============================================================
    # 2) Copy PROJECT_FOLDER -> C:\temp\src (excluding build, .git, etc.)
    # ============================================================
    Write-Host "==> Copying '$ProjectFolder' -> '$SrcFolder'"
    Write-Host "    (excluding: $($ExcludeDirs -join ', '), $($ExcludeFiles -join ', '))..."
    Invoke-RobocopySafe -Source $ProjectFolder -Destination $SrcFolder -XD $ExcludeDirs -XF $ExcludeFiles

    # Safety net: make sure no 'build' folder slipped through
    $leftoverBuild = Join-Path $SrcFolder "build"
    if (Test-Path $leftoverBuild) {
        Write-Host "==> Removing leftover 'build' folder in source copy..."
        Remove-Item -Path $leftoverBuild -Recurse -Force
    }

    # ============================================================
    # 3) CMake configure + build (Release)
    # ============================================================
    Write-Host "==> Running CMake configure..."

    Import-VcVarsEnvironment -VcVarsAllPath $VcVarsAllBat -Arch "x64"
    $env:PATH = "$JomDir;$env:PATH"

    & $CMakeExe `
        -S ($SrcFolder -replace '\\','/') `
        -B ($BuildDir  -replace '\\','/') `
        "-DCMAKE_BUILD_TYPE:STRING=Release" `
        "-DCMAKE_COLOR_DIAGNOSTICS:BOOL=ON" `
        "-DCMAKE_CXX_COMPILER:FILEPATH=$MsvcCompiler" `
        "-DCMAKE_C_COMPILER:FILEPATH=$MsvcCompiler" `
        "-DCMAKE_GENERATOR:STRING=NMake Makefiles JOM" `
        "-DCMAKE_PREFIX_PATH:PATH=C:/Qt/6.11.1/msvc2022_64" `
        "-DQT_ENABLE_QML_DEBUG:BOOL=OFF" `
        "-DQT_QMAKE_EXECUTABLE:FILEPATH=$($QMakeExe -replace '\\','/')"

    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed (exit code $LASTEXITCODE)" }

    Write-Host "==> Running CMake build (Release)..."
    & $CMakeExe --build ($BuildDir -replace '\\','/') --target all

    if ($LASTEXITCODE -ne 0) { throw "CMake build failed (exit code $LASTEXITCODE)" }

    # ============================================================
    # 4) Locate the built exe and copy it into C:\temp\deploy
    # ============================================================
    Write-Host "==> Locating $ExeName under $BuildDir..."
    $exeFile = Get-ChildItem -Path $BuildDir -Filter $ExeName -Recurse -File | Select-Object -First 1

    if (-not $exeFile) {
        throw "Could not find $ExeName anywhere under $BuildDir"
    }

    Write-Host "==> Found exe at: $($exeFile.FullName)"
    Write-Host "==> Copying exe to $DeployDir..."
    Copy-Item -Path $exeFile.FullName -Destination $DeployDir -Force

    # Also bring along the resources folder
    $resourcesSrc = Join-Path $SrcFolder "resources"
    if (Test-Path $resourcesSrc) {
        Write-Host "==> Copying resources folder into deployment..."
        $resourcesDest = Join-Path $DeployDir "resources"
        New-Item -ItemType Directory -Path $resourcesDest -Force | Out-Null
        Invoke-RobocopySafe -Source $resourcesSrc -Destination $resourcesDest
    } else {
        Write-Host "==> No 'resources' folder found at $resourcesSrc, skipping."
    }

    # ============================================================
    # 5) Run windeployqt
    # ============================================================
    Write-Host "==> Running windeployqt..."
    $deployedExe = Join-Path $DeployDir $ExeName
    & $WindeployqtExe $deployedExe --release

    if ($LASTEXITCODE -ne 0) { throw "windeployqt failed (exit code $LASTEXITCODE)" }

    # ============================================================
    # 6) Zip deployment folder (no wrapping folder) -> DEPLOYMENT_FILE
    # ============================================================
    Write-Host "==> Creating zip at $DeploymentFile..."

    $deploymentParent = Split-Path -Path $DeploymentFile -Parent
    if ($deploymentParent -and -not (Test-Path $deploymentParent)) {
        New-Item -ItemType Directory -Path $deploymentParent -Force | Out-Null
    }

    if (Test-Path $DeploymentFile) {
        Write-Host "==> Removing existing file at $DeploymentFile..."
        Remove-Item -Path $DeploymentFile -Force
    }

    # Zipping "$DeployDir\*" (not $DeployDir itself) avoids a wrapping folder in the archive
    Compress-Archive -Path (Join-Path $DeployDir "*") -DestinationPath $DeploymentFile

    Write-Host "==> Deployment zip created: $DeploymentFile"

    # ============================================================
    # Cleanup: delete C:\temp entirely on success
    # ============================================================
    Write-Host "==> Cleaning up $TempRoot..."
    Remove-Item -Path $TempRoot -Recurse -Force

    Write-Host "==> DONE. Deployment succeeded: $DeploymentFile"
}
catch {
    Write-Error "Build/deploy failed: $_"
    Write-Host "==> $TempRoot was left in place for debugging (not deleted due to failure)."
    exit 1
}
