# INSTALL & CHECK
# 
# > Native:
# - OpenSSL
# 
# > AVR:
# - Micro-ECC
# - Arduinolibs

$LIBS_DIR = "./libs"
$OPENSSL_PATH = "C:/Program Files/OpenSSL-Win64/bin/openssl.exe"

$LIBS = @(
	@{	path = "$LIBS_DIR/micro-ecc"
		url = "https://github.com/kmackay/micro-ecc.git"
	},
	@{	path = "$LIBS_DIR/arduinolibs"
		url = "https://github.com/rweather/arduinolibs.git"
	}
)

function Download-Lib {
	param([string]$url)
	$current = Get-Location
	Set-Location $LIBS_DIR

	git clone $url

	Set-Location $current
}

# ---------------------------
# Check Git
# ---------------------------

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "Git not installed or not installed in environtment path."
    exit 1
}

# ---------------------------
# Create libs directory
# ---------------------------

if (-not (Test-Path $LIBS_DIR)) {
    New-Item -ItemType Directory -Path $LIBS_DIR | Out-Null
	Write-Host "Creating $LIBS_DIR..."
}

# ---------------------------
# Download libraries
# ---------------------------

foreach ($lib in $LIBS) {
    if (-not (Test-Path $lib.path)) {
        Write-Host "Downloading $($lib.path)..."
        Download-Lib $lib.url
    }
    else {
        Write-Host "$($lib.path) already exists."
    }
}

# ---------------------------
# Check OpenSSL
# ---------------------------
if (-not (Test-Path $OPENSSL_PATH)) {

    $IsAdmin = ([Security.Principal.WindowsPrincipal] `
        [Security.Principal.WindowsIdentity]::GetCurrent()
    ).IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)

    if (-not $IsAdmin) {
        Write-Host "Requesting Administrator privileges..."
        Start-Process powershell `
            -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" `
            -Verb RunAs
        exit
    }

    Write-Host "Installing OpenSSL via winget..."
    winget install ShiningLight.OpenSSL.Dev
}
else {
    Write-Host "OpenSSL already installed."
}