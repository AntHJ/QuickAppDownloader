# ================================
#  AUTO‑ELEVATE TO ADMIN
# ================================
$IsAdmin = ([Security.Principal.WindowsPrincipal] `
    [Security.Principal.WindowsIdentity]::GetCurrent()
).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $IsAdmin) {
    Write-Host "Requesting administrator privileges..."

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = "powershell.exe"
    $psi.Arguments = "-NoProfile -ExecutionPolicy Bypass -Command `"irm '$($MyInvocation.MyCommand.Definition)' | iex`""
    $psi.Verb = "runas"

    try {
        [System.Diagnostics.Process]::Start($psi) | Out-Null
    } catch {
        Write-Host "User declined elevation. Exiting."
    }

    exit
}

# ================================
#  CONFIGURATION
# ================================
$zipUrl = "https://github.com/AntHJ/SD2Vita-Format-Tool/releases/download/1.2/SD2Vita.Format.Tool.v1.2.zip"
$zipPath = Join-Path $env:TEMP "package.zip"
$extractPath = Join-Path $env:TEMP "myinstaller"
$batFile = "SD2Vita Format Tool.bat"

# ================================
#  CLEAN PREVIOUS INSTALLER
# ================================
if (Test-Path $extractPath) {
    Remove-Item $extractPath -Recurse -Force
}

if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

# ================================
#  DOWNLOAD PACKAGE
# ================================
Write-Host "Downloading..."
Invoke-WebRequest -Uri $zipUrl -OutFile $zipPath

# ================================
#  EXTRACT PACKAGE
# ================================
Write-Host "Extracting..."
Expand-Archive -Path $zipPath -DestinationPath $extractPath -Force

# ================================
#  RUN INSTALLER
# ================================
$batPath = Join-Path $extractPath $batFile

if (Test-Path $batPath) {
    Write-Host "Running SD2Vita Format Tool..."
    Start-Process $batPath -Wait
} else {
    Write-Host "ERROR: '$batFile' not found inside package.zip"
    exit 1
}

# ================================
#  OPTIONAL CLEANUP
# ================================
# Remove-Item $zipPath -Force
# Remove-Item $extractPath -Recurse -Force

Write-Host "finished."
