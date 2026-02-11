$LIBS_DIR = "./libs"

# Cek apakah git terinstall
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "Git not installed or no installed in environtment path."
    exit 1
}

if (-not (Test-Path $LIBS_DIR)) {
    New-Item -ItemType Directory -Path $LIBS_DIR | Out-Null
	Set-Location $LIBS_DIR
	git clone https://github.com/kmackay/micro-ecc.git
	git clone https://github.com/rweather/arduinolibs.git
}

if (-not ([Security.Principal.WindowsPrincipal] `
    [Security.Principal.WindowsIdentity]::GetCurrent()
).IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)) {
    Write-Error "Must run as Administrator!"
    exit 1
}

if (-not $IsAdmin) {
    Write-Host "Request administrator privileges..."
    Start-Process powershell `
        -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" `
        -Verb RunAs
    exit
}

winget install winget ShiningLight.OpenSSL.Dev