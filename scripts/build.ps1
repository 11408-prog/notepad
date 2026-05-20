param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Debug",

    [string]$BuildDir = "build/cmake",

    [string]$Generator = "MinGW Makefiles"
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildPath = Join-Path $repoRoot $BuildDir

cmake -S $repoRoot -B $buildPath -G $Generator -DCMAKE_BUILD_TYPE=$Configuration
cmake --build $buildPath --config $Configuration --parallel
