[CmdletBinding()]
param(
    [Parameter()]
    [string]$ExpectedVersion = '0.1.0'
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Require-Match {
    param([string]$Path, [string]$Pattern, [string]$Message)
    $absolute = Join-Path $repo $Path
    if (-not (Select-String -LiteralPath $absolute -Pattern $Pattern -Quiet)) {
        throw $Message
    }
}

Require-Match 'gradle.properties' "^VERSION_NAME=$([regex]::Escape($ExpectedVersion))$" `
    "VERSION_NAME must be $ExpectedVersion"
Require-Match 'gradle.properties' '^GROUP=info\.marcin\.usbhost$' `
    'Maven group must be info.marcin.usbhost'
Require-Match 'usbHostForAndroid/build.gradle' "namespace 'info\.marcin\.usbhost'" `
    'Android library namespace is incorrect'
Require-Match '.github/workflows/release.yml' 'merge-base --is-ancestor.*origin/main' `
    'Release workflow must verify main containment'
Require-Match '.github/workflows/release.yml' 'environment: release' `
    'Release publication must use the protected release environment'

$activeRoots = @(
    'usbHostForAndroid/src',
    'usbHostForAndroid/consumer-rules.pro',
    'usbHostExample/src'
)
$legacy = Get-ChildItem -Path ($activeRoots | ForEach-Object { Join-Path $repo $_ }) -Recurse -File |
    Select-String -Pattern 'dev\.usbhost\.android|Java_dev_usbhost_android'
if ($legacy) {
    throw "Previous development namespace remains in active source: $($legacy.Path | Select-Object -Unique)"
}

$required = @(
    '.github/workflows/ci.yml',
    '.github/workflows/codeql.yml',
    '.github/workflows/dependency-review.yml',
    '.github/workflows/release.yml',
    '.github/workflows/runner-image.yml',
    'docker/android-runner/Dockerfile',
    'RELEASING.md'
)
foreach ($path in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $repo $path) -PathType Leaf)) {
        throw "Required release file is missing: $path"
    }
}

Write-Host "Release policy verified for $ExpectedVersion. No publication was attempted."
