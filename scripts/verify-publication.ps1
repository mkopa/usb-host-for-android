[CmdletBinding()]
param([string]$RepositoryRoot)

$ErrorActionPreference = 'Stop'
if (-not $RepositoryRoot) { $RepositoryRoot = Split-Path -Parent $PSScriptRoot }
$verifiedAar = Join-Path $RepositoryRoot 'usbHostForAndroid\build\verified-aar'
$sdk = if ($env:ANDROID_SDK_ROOT) { $env:ANDROID_SDK_ROOT } else { $env:ANDROID_HOME }
if (-not $sdk) {
    $localProperties = Join-Path $RepositoryRoot 'local.properties'
    if (Test-Path -LiteralPath $localProperties -PathType Leaf) {
        $sdkLine = Get-Content -LiteralPath $localProperties |
            Where-Object { $_ -like 'sdk.dir=*' } | Select-Object -First 1
        if ($sdkLine) {
            $sdk = $sdkLine.Substring('sdk.dir='.Length).Replace('\:', ':').Replace('\\', '\')
        }
    }
}
if (-not $sdk) { throw 'ANDROID_SDK_ROOT or ANDROID_HOME is required for symbol inspection.' }
$buildFile = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'usbHostForAndroid\build.gradle') -Raw
$ndkVersionMatch = [regex]::Match($buildFile, "ndkVersion\s+'([^']+)'")
$ndk = if ($ndkVersionMatch.Success) {
    Get-Item -LiteralPath (Join-Path $sdk "ndk\$($ndkVersionMatch.Groups[1].Value)") `
        -ErrorAction SilentlyContinue
} else { $null }
if (-not $ndk) {
    $ndk = Get-ChildItem -LiteralPath (Join-Path $sdk 'ndk') -Directory |
        Sort-Object Name -Descending | Select-Object -First 1
}
if (-not $ndk) { throw 'No Android NDK installation was found.' }
$nm = Join-Path $ndk.FullName 'toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-nm.exe'
if (-not (Test-Path -LiteralPath $nm -PathType Leaf)) { throw "llvm-nm is absent: $nm" }
$required = @(
    'usbhost_abi_version', 'usbhost_open_stlink_v3_fd', 'usbhost_connect_target',
    'usbhost_read_memory', 'usbhost_close', 'usbhost_status_name', 'usbhost_last_status',
    'usbhost_last_error', 'usbhost_transport_open_fd', 'usbhost_transport_cancel',
    'usbhost_transport_close', 'usbhost_transport_get_device_descriptor',
    'usbhost_transport_get_configuration_count', 'usbhost_transport_get_configuration_at',
    'usbhost_transport_get_interface_count', 'usbhost_transport_get_interface_at',
    'usbhost_transport_get_alternate_setting_count',
    'usbhost_transport_get_alternate_setting_at', 'usbhost_transport_get_endpoint_count',
    'usbhost_transport_get_endpoint_at', 'usbhost_transport_get_additional_descriptor_at',
    'usbhost_transport_select_configuration', 'usbhost_transport_claim_interface',
    'usbhost_transport_select_alternate_setting', 'usbhost_transport_release_interface',
    'usbhost_transport_control_transfer', 'usbhost_transport_bulk_transfer',
    'usbhost_transport_interrupt_transfer')

$baseline = Get-Content -LiteralPath (
    Join-Path $RepositoryRoot 'native-tests\public-symbols-baseline.txt')
$baselineSymbols = @($baseline | Where-Object {
    $_ -match '^usbhost_[a-z0-9_]+$'
})
foreach ($symbol in $baselineSymbols) {
    if ($required -notcontains $symbol) {
        throw "Previous STLINK ABI symbol is missing from the approved surface: $symbol"
    }
}

$abis = @('arm64-v8a', 'armeabi-v7a', 'x86_64')
$inspected = 0
foreach ($abi in $abis) {
    $libraries = @(
        (Join-Path $verifiedAar "jni\$abi\libusbhost.so"),
        (Join-Path $verifiedAar "prefab\modules\usbhost\libs\android.$abi\libusbhost.so")
    )
    foreach ($library in $libraries) {
        if (-not (Test-Path -LiteralPath $library -PathType Leaf)) {
            throw "Published native library is absent: $library"
        }
        $output = & $nm --dynamic --defined-only $library
        if ($LASTEXITCODE -ne 0) {
            throw "llvm-nm failed for $library with exit code $LASTEXITCODE."
        }
        $symbols = @($output | ForEach-Object {
            (($_ -split '\s+')[-1]) -replace '@@USBHOST_[0-9.]+$', ''
        } | Where-Object { $_ })
        foreach ($symbol in $required) {
            if ($symbols -notcontains $symbol) {
                throw "Required ABI symbol is absent from $library`: $symbol"
            }
        }
        $unexpected = $symbols | Where-Object {
            ($_ -like 'usbhost_*' -and $_ -notin $required) `
                -or $_ -like 'libusb_*' -or $_ -match '^_Z'
        }
        if ($unexpected) {
            throw "Unexpected public native symbols in $library`: $($unexpected -join ', ')"
        }
        ++$inspected
    }
}

$sourceHeaders = Join-Path $RepositoryRoot 'usbHostForAndroid\src\main\cpp\include\usbhost'
$publishedHeaders = Join-Path $verifiedAar 'prefab\modules\usbhost\include\usbhost'
foreach ($headerName in @('usbhost.h', 'transport.h')) {
    $sourceHeader = Join-Path $sourceHeaders $headerName
    $publishedHeader = Join-Path $publishedHeaders $headerName
    if (-not (Test-Path -LiteralPath $publishedHeader -PathType Leaf) -or
            (Get-FileHash -LiteralPath $sourceHeader).Hash -ne
            (Get-FileHash -LiteralPath $publishedHeader).Hash) {
        throw "Published Prefab header is absent or stale: usbhost/$headerName"
    }
}

$umbrellaHeader = Get-Content -LiteralPath (Join-Path $publishedHeaders 'usbhost.h') -Raw
foreach ($statusLine in ($baseline | Where-Object { $_ -match '^USBHOST_[A-Z_]+=[0-9]+$' })) {
    $name, $value = $statusLine -split '=', 2
    if ($umbrellaHeader -notmatch "(?m)\b$([regex]::Escape($name))\s*=\s*$value\b") {
        throw "Previous STLINK status is absent or renumbered: $statusLine"
    }
}

Write-Host "Verified $($required.Count) stable public C ABI symbols in $inspected JNI/Prefab libraries, headers, and the previous STLINK baseline."
