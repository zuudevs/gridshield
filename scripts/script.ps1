<# HEADER
FILE: script.ps1
AUTHOR: zuu
BRIEF: Automation script for GridShield Firmware (ESP-IDF + QEMU)
VERSION: 2.0.0
DATE: 2026-02-23
HEADER #>

$firmwareDir = Join-Path $PSScriptRoot "..\firmware"

# ============================================================================
# ESP-IDF Environment
# ============================================================================
function Activate-Idf {
    Write-Host "--- Activating ESP-IDF Environment ---" -ForegroundColor Cyan

    if (Test-Path "C:\esp\v5.5.3\esp-idf\export.ps1") {
        . C:\esp\v5.5.3\esp-idf\export.ps1
    } else {
        Write-Error "ESP-IDF not found at C:\esp\v5.5.3\esp-idf! Check your installation."
        return $false
    }

    # Inject MinGW Git path for QEMU dependencies (libgcrypt, libglib, etc.)
    if ($env:PATH -notlike "*D:\dev-tools\git\mingw64\bin*") {
        $env:PATH = "D:\dev-tools\git\mingw64\bin;" + $env:PATH
        Write-Host "--- Git MinGW Path Injected ---" -ForegroundColor Yellow
    }

    return $true
}

# ============================================================================
# Build Firmware
# ============================================================================
function Invoke-Build {
    Set-Location $firmwareDir
    if (-not (Activate-Idf)) { return }

    Write-Host "`n[BUILD] Compiling GridShield firmware...`n" -ForegroundColor Green
    idf.py build
}

# ============================================================================
# Run in QEMU (with IDF Monitor — colored output + serial input)
# ============================================================================
function Invoke-Run {
    Set-Location $firmwareDir
    if (-not (Activate-Idf)) { return }

    Write-Host "`n[QEMU] Starting GridShield simulation...`n" -ForegroundColor Green
    Write-Host "  Exit: Ctrl+]" -ForegroundColor DarkGray
    Write-Host ""

    idf.py qemu monitor
}

# ============================================================================
# Run in QEMU (raw mode — direct QEMU console, no IDF monitor)
# ============================================================================
function Invoke-RunRaw {
    Set-Location $firmwareDir
    if (-not (Activate-Idf)) { return }

    Write-Host "`n[QEMU] Starting raw QEMU (no IDF monitor)...`n" -ForegroundColor Green
    Write-Host "  QEMU console: Ctrl+A then C" -ForegroundColor DarkGray
    Write-Host "  Exit:         Ctrl+A then Q" -ForegroundColor DarkGray
    Write-Host ""

    idf.py qemu
}

# ============================================================================
# Debug with GDB (two-terminal workflow)
# ============================================================================
function Invoke-Debug {
    Set-Location $firmwareDir
    if (-not (Activate-Idf)) { return }

    Write-Host "`n[DEBUG] Starting QEMU with GDB server...`n" -ForegroundColor Magenta
    Write-Host "  In another terminal, run:" -ForegroundColor Yellow
    Write-Host "    .\scripts\script.ps1 --gdb" -ForegroundColor White
    Write-Host ""

    idf.py qemu --gdb monitor
}

# ============================================================================
# Attach GDB (run in second terminal)
# ============================================================================
function Invoke-Gdb {
    Set-Location $firmwareDir
    if (-not (Activate-Idf)) { return }

    Write-Host "`n[GDB] Connecting to QEMU...`n" -ForegroundColor Magenta
    idf.py gdb
}

# ============================================================================
# Install QEMU (one-time setup)
# ============================================================================
function Invoke-Setup {
    if (-not (Activate-Idf)) { return }

    Write-Host "`n[SETUP] Installing QEMU for ESP32 (Xtensa)...`n" -ForegroundColor Cyan
    python "$env:IDF_PATH\tools\idf_tools.py" install qemu-xtensa qemu-riscv32

    Write-Host "`n[SETUP] Re-exporting PATH...`n" -ForegroundColor Cyan
    . C:\esp\v5.5.3\esp-idf\export.ps1

    Write-Host "`n[SETUP] Verifying QEMU installation..." -ForegroundColor Cyan
    $qemuPath = Get-Command qemu-system-xtensa -ErrorAction SilentlyContinue
    if ($qemuPath) {
        Write-Host "[OK] qemu-system-xtensa found at: $($qemuPath.Source)" -ForegroundColor Green
    } else {
        Write-Host "[WARN] qemu-system-xtensa not found in PATH. Try reopening the terminal." -ForegroundColor Yellow
    }
}

# ============================================================================
# Clean build
# ============================================================================
function Invoke-Clean {
    Set-Location $firmwareDir
    if (-not (Activate-Idf)) { return }

    Write-Host "`n[CLEAN] Removing build artifacts...`n" -ForegroundColor Yellow
    idf.py fullclean
}

# ============================================================================
# CLI Router
# ============================================================================
function Show-Help {
    Write-Host ""
    Write-Host "GridShield Firmware CLI" -ForegroundColor Cyan
    Write-Host "======================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  Usage: .\scripts\script.ps1 <command>" -ForegroundColor White
    Write-Host ""
    Write-Host "  Commands:" -ForegroundColor Yellow
    Write-Host "    -e,  --env       Open shell with ESP-IDF environment activated"
    Write-Host "    -b,  --build     Build firmware"
    Write-Host "    -r,  --run       Build + Run in QEMU (with IDF Monitor)"
    Write-Host "    -rr, --run-raw   Build + Run in QEMU (raw console)"
    Write-Host "    -d,  --debug     Build + Run in QEMU with GDB server"
    Write-Host "    -g,  --gdb       Attach GDB to running QEMU (second terminal)"
    Write-Host "    -c,  --clean     Full clean build artifacts"
    Write-Host "    -s,  --setup     Install QEMU (one-time)"
    Write-Host "    -h,  --help      Show this help"
    Write-Host ""
}

if ($args.Count -eq 0) {
    Show-Help
    exit
}

switch -Wildcard ($args[0]) {
    { $_ -eq "-e" -or $_ -eq "--env" } {
        powershell -NoExit -Command "& { . '$PSScriptRoot\script.ps1'; Activate-Idf; Clear-Host; Write-Host 'ESP-IDF Environment Ready!' -ForegroundColor Green }"
    }
    { $_ -eq "-b" -or $_ -eq "--build" } {
        Invoke-Build
    }
    { $_ -eq "-r" -or $_ -eq "--run" } {
        Invoke-Run
    }
    { $_ -eq "-rr" -or $_ -eq "--run-raw" } {
        Invoke-RunRaw
    }
    { $_ -eq "-d" -or $_ -eq "--debug" } {
        Invoke-Debug
    }
    { $_ -eq "-g" -or $_ -eq "--gdb" } {
        Invoke-Gdb
    }
    { $_ -eq "-c" -or $_ -eq "--clean" } {
        Invoke-Clean
    }
    { $_ -eq "-s" -or $_ -eq "--setup" } {
        Invoke-Setup
    }
    { $_ -eq "-h" -or $_ -eq "--help" } {
        Show-Help
    }
    default {
        Write-Host "Invalid Option: '$($args[0])'" -ForegroundColor Red
        Show-Help
    }
}