[CmdletBinding()]
param([string]$RepositoryRoot)

$ErrorActionPreference = 'Stop'
if (-not $RepositoryRoot) { $RepositoryRoot = Split-Path -Parent $PSScriptRoot }
$library = Join-Path $RepositoryRoot 'usbHostForAndroid\build\verified-aar\jni\arm64-v8a\libusbhost.so'
if (-not (Test-Path -LiteralPath $library -PathType Leaf)) {
    throw "Published native library is absent: $library"
}
$sdk = if ($env:ANDROID_SDK_ROOT) { $env:ANDROID_SDK_ROOT } else { $env:ANDROID_HOME }
if (-not $sdk) { throw 'ANDROID_SDK_ROOT or ANDROID_HOME is required for symbol inspection.' }
$ndk = Get-ChildItem -LiteralPath (Join-Path $sdk 'ndk') -Directory |
    Sort-Object Name -Descending | Select-Object -First 1
if (-not $ndk) { throw 'No Android NDK installation was found.' }
$nm = Join-Path $ndk.FullName 'toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-nm.exe'
if (-not (Test-Path -LiteralPath $nm -PathType Leaf)) { throw "llvm-nm is absent: $nm" }
$output = & $nm --dynamic --defined-only $library
if ($LASTEXITCODE -ne 0) { throw "llvm-nm failed with exit code $LASTEXITCODE." }
$symbols = @($output | ForEach-Object {
    (($_ -split '\s+')[-1]) -replace '@@USBHOST_[0-9.]+$', ''
} | Where-Object { $_ })
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
foreach ($symbol in $required) {
    if ($symbols -notcontains $symbol) { throw "Required ABI symbol is absent: $symbol" }
}
$unexpected = $symbols | Where-Object {
    ($_ -like 'usbhost_*' -and $_ -notin $required) -or $_ -like 'libusb_*' -or $_ -match '^_Z'
}
if ($unexpected) { throw "Unexpected public native symbols: $($unexpected -join ', ')" }
Write-Host "Verified $($required.Count) stable public C ABI symbols in the published AAR."
