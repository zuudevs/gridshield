<# HEADER
FILE: structure.ps1
AUTHOR: zuudevs
EMAIL: zuudevs@gmail.com
BRIEF: Display the directory tree of a project with optional exclude folders
VERSION: 0.0.5
DATE: 2026-02-08 09:15:00
COPYRIGHT: Copyright (c) 2026 zuudevs
HEADER #>

function Show-Tree {
    param(
        [string]$Path,
        [string[]]$ExcludeFolders = @(),
        [string]$Prefix = ""
    )

    if (-not (Test-Path $Path)) {
        Write-Host "Path '$Path' not found."
        return
    }

    $items = Get-ChildItem -LiteralPath $Path | Where-Object { 
        ($_.PSIsContainer -and ($ExcludeFolders -notcontains $_.Name)) -or -not $_.PSIsContainer
    }

    $count = $items.Count
    for ($i = 0; $i -lt $count; $i++) {
        $item = $items[$i]
        $isLast = ($i -eq $count - 1)

        if ($item.PSIsContainer) {
            Write-Host "$Prefix+ $($item.Name)"
            
            # PERBAIKAN: Menangani logika if di luar assignment string
            # Menggunakan [char]0x2502 agar kompatibel dengan semua versi PowerShell
            $connector = "$([char]0x2502) " 
            if ($isLast) { 
                $connector = "  " 
            }
            
            $newPrefix = $Prefix + $connector
            Show-Tree -Path $item.FullName -ExcludeFolders $ExcludeFolders -Prefix $newPrefix
        } else {
            Write-Host "$Prefix- $($item.Name)"
        }
    }
}

# ================================
# Parse Command-Line Arguments
# ================================
$TargetPath = $null
$ExcludeFolders = @()
$collectingExclude = $false

foreach ($arg in $args) {
    switch ($arg) {
        "-e" { $collectingExclude = $true; continue }
        "--exclude" { $collectingExclude = $true; continue }
        default {
            if ($collectingExclude) {
                $ExcludeFolders += $arg
            } elseif (-not $TargetPath) {
                $TargetPath = $arg
            } else {
                Write-Host "Unknown argument: $arg"
                exit 1
            }
        }
    }
}

if (-not $TargetPath) {
    Write-Host "Usage: .\structure.ps1 [target-path] -e folder1 folder2 ..."
    exit 1
}

# Jalankan tree
Show-Tree -Path $TargetPath -ExcludeFolders $ExcludeFolders