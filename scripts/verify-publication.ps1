[CmdletBinding()]
param([string]$RepositoryRoot)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $RepositoryRoot) { $RepositoryRoot = Split-Path -Parent $PSScriptRoot }
$RepositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
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

$classesJar = Join-Path $verifiedAar 'classes.jar'
if (-not (Test-Path -LiteralPath $classesJar -PathType Leaf)) {
    throw 'Published AAR classes.jar is absent.'
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::OpenRead($classesJar)
try {
    $classEntries = @($archive.Entries | ForEach-Object FullName)
} finally {
    $archive.Dispose()
}
$managedBaseline = Get-Content -LiteralPath (
    Join-Path $RepositoryRoot 'usbHostForAndroid\src\test\resources\public-managed-api-baseline.txt') |
    Where-Object { $_ -match '^(?:class|enum)\s+(.+)$' } |
    ForEach-Object { ($Matches[1] -replace '\.', '/') + '.class' }
$transportClasses = @(
    'AdditionalUsbDescriptor', 'GenericUsbAlternateSetting', 'GenericUsbConfiguration',
    'GenericUsbDevice', 'GenericUsbDeviceDescriptor', 'GenericUsbEndpoint',
    'GenericUsbInterface', 'GenericUsbInterfaceDescriptor', 'UsbControlRequest',
    'UsbDirection', 'UsbTransferResult', 'UsbTransferType', 'UsbTransportException',
    'UsbTransportStatus'
) | ForEach-Object { "info/marcin/usbhost/transport/$_.class" }
foreach ($managedClass in @($managedBaseline) + @($transportClasses)) {
    if ($classEntries -notcontains $managedClass) {
        throw "Required managed API class is absent from the AAR: $managedClass"
    }
}

$pom = Get-ChildItem -LiteralPath (
    Join-Path $RepositoryRoot 'usbHostForAndroid\build\repository\info\marcin\usbhost\usb-host-for-android\0.1.0') `
    -Filter '*.pom' -File | Select-Object -First 1
if (-not $pom) { throw 'Local Maven POM is absent.' }
[xml]$pomXml = Get-Content -LiteralPath $pom.FullName -Raw
$namespace = [System.Xml.XmlNamespaceManager]::new($pomXml.NameTable)
$namespace.AddNamespace('m', 'http://maven.apache.org/POM/4.0.0')
$publishedDependencies = @($pomXml.SelectNodes('//m:dependency', $namespace) | ForEach-Object {
    "$($_.groupId):$($_.artifactId):$($_.version):$($_.scope)"
})
$approvedPublishedDependencies = @('org.jetbrains.kotlin:kotlin-stdlib:2.3.21:compile')
$dependencyDifference = @(Compare-Object $approvedPublishedDependencies $publishedDependencies)
if ($dependencyDifference.Count -ne 0) {
    throw "Published runtime dependency set changed: $($publishedDependencies -join ', ')"
}

$expectedSubmodules = @{
    'third_party/libusb' = 'https://github.com/libusb/libusb.git'
    'third_party/stlink' = 'https://github.com/stlink-org/stlink.git'
}
$configuredSubmodulePaths = @(& git -C $RepositoryRoot config --file .gitmodules --get-regexp '\.path$')
if ($LASTEXITCODE -ne 0) { throw 'Unable to inspect Git submodule paths.' }
$observedSubmodules = @{}
foreach ($entry in $configuredSubmodulePaths) {
    $key, $path = $entry -split '\s+', 2
    $section = $key -replace '\.path$', ''
    $url = (& git -C $RepositoryRoot config --file .gitmodules --get "$section.url").Trim()
    $observedSubmodules[$path] = $url
}
if ($observedSubmodules.Count -ne $expectedSubmodules.Count) {
    throw 'The tracked third-party submodule set changed.'
}
foreach ($path in $expectedSubmodules.Keys) {
    if (-not $observedSubmodules.ContainsKey($path) -or
            $observedSubmodules[$path] -ne $expectedSubmodules[$path]) {
        throw "Third-party submodule provenance changed: $path"
    }
}

$allowedGradleDependencies = @(
    "implementation project(':usbHostForAndroid')", 'implementation composeBom',
    'def composeBom = platform("androidx.compose:compose-bom:${providers.gradleProperty(''COMPOSE_BOM_VERSION'').get()}")',
    "implementation 'androidx.activity:activity-compose:1.13.0'",
    "implementation 'androidx.compose.material3:material3'",
    "implementation 'androidx.compose.ui:ui'",
    "implementation 'androidx.compose.ui:ui-tooling-preview'",
    "debugImplementation 'androidx.compose.ui:ui-tooling'",
    "implementation 'info.marcin.usbhost:usb-host-for-android:0.1.0'",
    "testImplementation 'junit:junit:4.13.2'"
)
$gradleDependencyLines = @(& git -C $RepositoryRoot grep -h -E `
    '^\s*((api|implementation|compileOnly|runtimeOnly|debugImplementation|testImplementation|androidTestImplementation)\s+|def composeBom = platform)' `
    -- '*.gradle' '*.gradle.kts' 2>$null | ForEach-Object { $_.Trim() } | Sort-Object -Unique)
if ($LASTEXITCODE -gt 1) { throw 'Unable to audit Gradle dependency declarations.' }
foreach ($dependencyLine in $gradleDependencyLines) {
    if ($allowedGradleDependencies -notcontains $dependencyLine) {
        throw "Unapproved Gradle dependency declaration: $dependencyLine"
    }
}

$localPathPattern = @(
    ('C:' + '[\\/]' + 'Users' + '[\\/]'),
    ('/' + 'Users' + '/[^/]+' + '/'),
    ('/' + 'home' + '/[^/]+' + '/')
) -join '|'
if (-not [string]::IsNullOrWhiteSpace($env:USBHOST_PUBLIC_PROHIBITED_PATTERN)) {
    $localPathPattern += '|' + $env:USBHOST_PUBLIC_PROHIBITED_PATTERN
}
$publicFindings = @(& git -C $RepositoryRoot grep -I -n -E $localPathPattern -- . 2>$null)
$grepExit = $LASTEXITCODE
if ($grepExit -eq 0 -and $publicFindings.Count -gt 0) {
    throw "Tracked public content contains a prohibited path or configured private pattern: $($publicFindings -join '; ')"
}
if ($grepExit -gt 1) { throw 'Unable to audit tracked public content.' }

Write-Host "Verified public content, dependencies, managed API, headers, and $($required.Count) stable C ABI symbols in $inspected JNI/Prefab libraries."
