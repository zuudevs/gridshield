# =========================
# BUILD TYPES
# =========================
$build_types = @(
	"native-debug",
	"native-release",
	"avr-uno",
	"avr-mega",
	"esp32"
)

$build_types_desc = @{
	"native-debug"   = "PC Development + sanitizers"
	"native-release" = "PC Performance testing"
	"avr-uno"        = "ATmega328P Arduino Uno (limited)"
	"avr-mega"       = "ATmega2560 Arduino Mega (recommended)"
	"esp32"          = "Xtensa (future support)"
}

# =========================
# OPTIONS
# =========================
$options = @("--build", "--clean", "--compile", "--help")
$options_short = @("-b", "-c", "-p", "-h")
$options_desc = @(
	"Configure the project using a build preset.",
	"Clean build directory (all or specific preset).",
	"Compile the project using a build preset.",
	"Show this help message."
)

# =========================
# UTILITIES
# =========================
function getMaxStringLength {
	param ([string[]]$strings)
	($strings | Measure-Object Length -Maximum).Maximum
}

function getTimestamp {
	return (Get-Date).ToString("yyyyMMdd_HHmmss")
}

# =========================
# HELP & LISTING
# =========================
function print_options {
	$maxLong  = getMaxStringLength $options
	$maxShort = getMaxStringLength $options_short

	for ($i = 0; $i -lt $options.Count; $i++) {
		$long  = $options[$i].PadRight($maxLong + 2)
		$short = $options_short[$i].PadRight($maxShort + 2)
		Write-Host "  $short $long $($options_desc[$i])"
	}
}

function print_build_types {
	$maxLength = getMaxStringLength $build_types
	foreach ($type in $build_types) {
		$typePad = $type.PadRight($maxLength + 2)
		Write-Host "  $typePad $($build_types_desc[$type])"
	}
}

function help {
	Write-Host "Usage: scripts.ps1 [option] [build-type] [--export]"
	Write-Host ""
	Write-Host "Options:"
	print_options
	Write-Host ""
	Write-Host "Build types:"
	print_build_types
	Write-Host ""
	Write-Host "Extra:"
	Write-Host "  --export   Export logs to ./logs/<build-type>/"
}

# =========================
# VALIDATION
# =========================
function build_type_validator {
	param ([string]$arg)

	if (-not $arg) {
		Write-Host "Build type is required."
		return $false
	}

	if ($arg -eq "--list" -or $arg -eq "-l") {
		print_build_types
		return $false
	}

	if ($build_types -notcontains $arg) {
		Write-Host "Invalid build type: $arg"
		Write-Host "Use --list to see available build types."
		return $false
	}

	return $true
}

# =========================
# EXPORT
# =========================
function prepare_export_dir {
	param ([string]$buildType)

	$path = "./logs/$buildType"
	New-Item $path -ItemType Directory -Force | Out-Null
	return $path
}

# =========================
# COMMANDS
# =========================
function build {
	param (
		[string]$arg,
		[bool]$export = $false
	)

	if (-not (build_type_validator $arg)) { return }

	if ($export) {
		$out = prepare_export_dir $arg
		$ts  = getTimestamp
		$log = "$out/configure_$ts.log"

		Write-Host "Configuring preset: $arg (exporting)"
		cmake --preset $arg *> $log
		Write-Host "Log saved to $log"
	} else {
		Write-Host "Configuring preset: $arg"
		cmake --preset $arg
	}
}

function compile {
	param (
		[string]$arg,
		[bool]$export = $false
	)

	if (-not (build_type_validator $arg)) { return }

	if ($export) {
		$out = prepare_export_dir $arg
		$ts  = getTimestamp
		$log = "$out/build_$ts.log"

		Write-Host "Building preset: $arg (exporting)"
		cmake --build --preset $arg *> $log
		Write-Host "Log saved to $log"
	} else {
		Write-Host "Building preset: $arg"
		cmake --build --preset $arg
	}
}

function clean {
	param ([string]$arg = "")

	if (-not $arg) {
		Remove-Item -Recurse -Force ./build -ErrorAction SilentlyContinue
		New-Item ./build -ItemType Directory | Out-Null
		Write-Host "Cleaned all build files."
		return
	}

	if (-not (build_type_validator $arg)) { return }

	$dir = "./build/$arg"
	if (Test-Path $dir) {
		Remove-Item -Recurse -Force $dir
		New-Item $dir -ItemType Directory | Out-Null
		Write-Host "Cleaned build type: $arg"
	} else {
		Write-Host "No build directory for: $arg"
	}
}

function print_invalid_arg {
	Write-Host "Unknown command. Use --help to see usage."
}

# =========================
# ARG PARSING
# =========================
$exportFlag = $args -contains "--export"

switch ($args[0]) {
	"--build"   { build   $args[1] $exportFlag }
	"-b"        { build   $args[1] $exportFlag }
	"--compile" { compile $args[1] $exportFlag }
	"-p"        { compile $args[1] $exportFlag }
	"--clean"   { clean   $args[1] }
	"-c"        { clean   $args[1] }
	"--help"    { help }
	"-h"        { help }
	Default     { print_invalid_arg }
}
