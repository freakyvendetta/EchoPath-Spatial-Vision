# EchoPath — launch main.py (local YOLOv8 pipeline)
Write-Host "[EchoPath] Starting main.py with Python..." -ForegroundColor Cyan
$py = (Get-Command python -ErrorAction SilentlyContinue).Source
if (-not $py) {
    Write-Host "[EchoPath] Error: Python executable not found in PATH." -ForegroundColor Red
    exit 1
}
& $py "$PSScriptRoot\main.py"

