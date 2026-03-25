# Direct download URL
$URL = 'https://github.com/AntHJ/SD2Vita-Format-Tool/releases/download/1.2/SD2Vita.Format.Tool.v1.2.zip'

# Create temp folder for extraction
$ExtractPath = Join-Path $env:TEMP "SD2VitaTool_$([guid]::NewGuid().Guid)"
New-Item -ItemType Directory -Path $ExtractPath | Out-Null

# Download ZIP
Write-Host "Downloading SD2Vita Format Tool..."
try {
    $zipPath = Join-Path $env:TEMP "SD2VitaTool.zip"
    Invoke-WebRequest -Uri $URL -OutFile $zipPath -UseBasicParsing
}
catch {
    Write-Host "Download failed: $($_.Exception.Message)" -ForegroundColor Red
    exit
}

Write-Host "Extracting..."
try {
    Expand-Archive -Path $zipPath -DestinationPath $ExtractPath -Force
}
catch {
    Write-Host "Extraction failed: $($_.Exception.Message)" -ForegroundColor Red
    exit
}

# Remove ZIP after extraction
Remove-Item $zipPath -Force

# Locate the BAT file
$BatFile = Join-Path $ExtractPath "SD2Vita Format Tool.bat"

if (-not (Test-Path $BatFile)) {
    Write-Host "Could not find 'SD2Vita Format Tool.bat' after extraction." -ForegroundColor Red
    exit
}

Write-Host "Launching SD2Vita Format Tool as Administrator..."

# Launch BAT with admin privileges
Start-Process -FilePath $BatFile -Verb RunAs

# Self-delete this script
$Self = $MyInvocation.MyCommand.Path
Start-Sleep -Milliseconds 500
cmd /c "del `"$Self`""
