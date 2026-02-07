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

$options = @("--build", "--clean", "--compile", "--help")
$options_short = @("-b", "-c", "-p", "-h")
$options_desc = @(
	"Build (configure) the project with the specified build type.",
	"Clean the build directory or specific build type.",
	"Compile the project with the specified build type.",
	"Show this help message."
)

function getMaxStringLength {
	param ([string[]]$strings)
	($strings | Measure-Object Length -Maximum).Maximum
}

function print_options {
	$maxLong  = getMaxStringLength $options
	$maxShort = getMaxStringLength $options_short

	for ($i = 0; $i -lt $options.Count; $i++) {
		$long  = $options[$i].PadRight($maxLong + 2)
		$short = $options_short[$i].PadRight($maxShort + 2)
		Write-Host "  $short $long $($options_desc[$i])"
	}
}

function help {
	Write-Host "Usage: scripts.ps1 [option] [build-type]"
	Write-Host ""
	Write-Host "Options:"
	print_options
	Write-Host ""
	Write-Host "Build types:"
	print_build_types
}

function print_build_types {
	$maxLength = getMaxStringLength $build_types
	foreach ($type in $build_types) {
		$typePad = $type.PadRight($maxLength + 2)
		Write-Host "  $typePad $($build_types_desc[$type])"
	}
}

function build_type_validator {
	param ([string]$arg)

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

function build {
	param ([string]$arg)

	if (-not (build_type_validator $arg)) { return }
	Write-Host "Configuring build preset: $arg"
	cmake --preset $arg
}

function compile {
	param ([string]$arg)

	if (-not (build_type_validator $arg)) { return }
	Write-Host "Building preset: $arg"
	cmake --build --preset $arg
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

switch ($args[0]) {
	"--build"   { build $args[1] }
	"-b"        { build $args[1] }
	"--clean"   { clean $args[1] }
	"-c"        { clean $args[1] }
	"--compile" { compile $args[1] }
	"-p"        { compile $args[1] }
	"--help"    { help }
	"-h"        { help }
	Default     { print_invalid_arg }
}
