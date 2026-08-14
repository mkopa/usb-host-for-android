[CmdletBinding()]
param(
    [switch]$SkipNative,
    [switch]$SkipAndroid,
    [switch]$SkipPublication,
    [switch]$SkipGitHub,
    [string]$NativeBuildDirectory,
    [string]$Repository = 'mkopa/usb-host-for-android'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($NativeBuildDirectory)) {
    $NativeBuildDirectory = Join-Path $repo 'build\native-tests-sanitized'
}

function Invoke-Checked {
    param(
        [Parameter(Mandatory)]
        [string]$Command,
        [Parameter(Mandatory)]
        [string[]]$Arguments,
        [Parameter(Mandatory)]
        [string]$Label
    )

    Write-Host "`n==> $Label"
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Label failed with exit code $LASTEXITCODE."
    }
}

function Resolve-Executable {
    param(
        [Parameter(Mandatory)]
        [string[]]$Names
    )

    foreach ($name in $Names) {
        $command = Get-Command $name -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($command) {
            return $command.Source
        }
    }
    return $null
}

function Resolve-AndroidSdk {
    foreach ($candidate in @($env:ANDROID_SDK_ROOT, $env:ANDROID_HOME)) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and
                (Test-Path -LiteralPath $candidate -PathType Container)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    $properties = Join-Path $repo 'local.properties'
    if (Test-Path -LiteralPath $properties -PathType Leaf) {
        $sdkLine = Get-Content -LiteralPath $properties |
            Where-Object { $_ -match '^sdk\.dir=' } |
            Select-Object -First 1
        if ($sdkLine) {
            $candidate = $sdkLine.Substring($sdkLine.IndexOf('=') + 1)
            $candidate = $candidate.Replace('\:', ':').Replace('\\', '\')
            if (Test-Path -LiteralPath $candidate -PathType Container) {
                return (Resolve-Path -LiteralPath $candidate).Path
            }
        }
    }

    return $null
}

function Resolve-Java17Home {
    $candidates = [System.Collections.Generic.List[string]]::new()
    if (-not [string]::IsNullOrWhiteSpace($env:JAVA_HOME)) {
        $candidates.Add($env:JAVA_HOME)
    }

    $java = Resolve-Executable @('java.exe', 'java')
    if ($java) {
        $candidates.Add((Split-Path (Split-Path $java -Parent) -Parent))
    }

    if ($env:ProgramFiles) {
        foreach ($root in @(
            (Join-Path $env:ProgramFiles 'Java'),
            (Join-Path $env:ProgramFiles 'Eclipse Adoptium')
        )) {
            if (Test-Path -LiteralPath $root -PathType Container) {
                Get-ChildItem -LiteralPath $root -Directory -Filter '*17*' |
                    ForEach-Object { $candidates.Add($_.FullName) }
            }
        }
    }

    foreach ($candidate in @($candidates | Select-Object -Unique)) {
        $javaBinary = Join-Path $candidate 'bin\java.exe'
        if (-not (Test-Path -LiteralPath $javaBinary -PathType Leaf)) {
            $javaBinary = Join-Path $candidate 'bin/java'
        }
        if (Test-Path -LiteralPath $javaBinary -PathType Leaf) {
            $version = & $javaBinary -version 2>&1 | Select-Object -First 1
            if ([string]$version -match 'version "17(?:\.|\")') {
                return (Resolve-Path -LiteralPath $candidate).Path
            }
        }
    }

    return $null
}

function Resolve-Ninja {
    param([string]$AndroidSdk)

    $ninja = Resolve-Executable @('ninja.exe', 'ninja')
    if ($ninja) {
        return $ninja
    }

    if ($AndroidSdk) {
        $candidates = @(
            Get-ChildItem -LiteralPath (Join-Path $AndroidSdk 'cmake') -Directory `
                -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending |
                ForEach-Object { Join-Path $_.FullName 'bin\ninja.exe' }
        )
        foreach ($candidate in $candidates) {
            if (Test-Path -LiteralPath $candidate -PathType Leaf) {
                return $candidate
            }
        }
    }

    return $null
}

function Reset-NativeBuildDirectory {
    $repoBuildRoot = [System.IO.Path]::GetFullPath((Join-Path $repo 'build'))
    $requested = [System.IO.Path]::GetFullPath($NativeBuildDirectory)
    $separator = [System.IO.Path]::DirectorySeparatorChar
    $comparison = if ($env:OS -eq 'Windows_NT') {
        [System.StringComparison]::OrdinalIgnoreCase
    } else {
        [System.StringComparison]::Ordinal
    }
    if (-not $requested.StartsWith($repoBuildRoot.TrimEnd($separator) + $separator, $comparison)) {
        throw 'NativeBuildDirectory must be a generated child of the repository build directory.'
    }
    if (Test-Path -LiteralPath $requested) {
        Remove-Item -LiteralPath $requested -Recurse -Force
    }
}

function Assert-PublicPolicy {
    $author = (& git -C $repo config user.name).Trim()
    $email = (& git -C $repo config user.email).Trim()
    if ($author -ne 'Marci Kopa' -or $email -ne 'marcin@marcin.info') {
        throw 'Git author/committer configuration does not match the public repository policy.'
    }

    & git -C $repo diff --check
    if ($LASTEXITCODE -ne 0) {
        throw 'git diff --check failed.'
    }

    $windowsUserRoot = 'C:' + '[\\/]' + 'Users' + '[\\/]'
    $macUserRoot = '/' + 'Users' + '/[^/]+' + '/'
    $linuxUserRoot = '/' + 'home' + '/[^/]+' + '/'
    $privatePattern = @($windowsUserRoot, $macUserRoot, $linuxUserRoot) -join '|'
    if (-not [string]::IsNullOrWhiteSpace($env:USBHOST_PUBLIC_PROHIBITED_PATTERN)) {
        $privatePattern += '|' + $env:USBHOST_PUBLIC_PROHIBITED_PATTERN
    }
    $matches = @(& git -C $repo grep -I -n -E $privatePattern -- . 2>$null)
    $grepExit = $LASTEXITCODE
    if ($grepExit -eq 0 -and $matches.Count -gt 0) {
        $matches | ForEach-Object { Write-Error $_ }
        throw 'Public-content scan found prohibited private content or a local path.'
    }
    if ($grepExit -gt 1) {
        throw "Public-content scan failed with exit code $grepExit."
    }

    Write-Host 'Public repository policy verified.'
}

Push-Location $repo
try {
    $androidSdk = Resolve-AndroidSdk

    if (-not $SkipNative) {
        $cmake = Resolve-Executable @('cmake.exe', 'cmake')
        $ctest = Resolve-Executable @('ctest.exe', 'ctest')
        $clang = Resolve-Executable @('clang.exe', 'clang')
        $clangxx = Resolve-Executable @('clang++.exe', 'clang++')
        $ninja = Resolve-Ninja $androidSdk
        if (-not $cmake -or -not $ctest -or -not $clang -or -not $clangxx -or -not $ninja) {
            throw 'Native verification requires CMake, CTest, Clang, Clang++, and Ninja.'
        }

        $env:PATH = "$(Split-Path $clang -Parent);$env:PATH"
        Reset-NativeBuildDirectory
        $cmakeNinja = $ninja.Replace('\', '/')
        $cmakeClang = $clang.Replace('\', '/')
        $cmakeClangxx = $clangxx.Replace('\', '/')
        $sanitizerSet = if ($env:OS -eq 'Windows_NT') {
            Write-Host 'Windows host sanitizer: UBSan (ASan additionally requires a configured x64 MSVC STL runtime).'
            'undefined'
        } else {
            'address,undefined'
        }
        $sanitizers = "-fsanitize=$sanitizerSet -fno-omit-frame-pointer"
        Invoke-Checked $cmake @(
            '-S', (Join-Path $repo 'native-tests'),
            '-B', $NativeBuildDirectory,
            '-G', 'Ninja',
            "-DCMAKE_MAKE_PROGRAM=$cmakeNinja",
            "-DCMAKE_C_COMPILER=$cmakeClang",
            "-DCMAKE_CXX_COMPILER=$cmakeClangxx",
            '-DCMAKE_BUILD_TYPE=RelWithDebInfo',
            '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded',
            "-DCMAKE_C_FLAGS=$sanitizers",
            "-DCMAKE_CXX_FLAGS=$sanitizers",
            "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=$sanitizerSet"
        ) "Configure native tests with $sanitizerSet sanitizer(s)"
        Invoke-Checked $cmake @('--build', $NativeBuildDirectory, '--parallel', '2') `
            'Build native sanitizer tests'
        if ($sanitizerSet.Contains('address')) {
            $env:ASAN_OPTIONS = 'halt_on_error=1'
        }
        $env:UBSAN_OPTIONS = 'halt_on_error=1:print_stacktrace=1'
        Invoke-Checked $ctest @(
            '--test-dir', $NativeBuildDirectory,
            '--output-on-failure'
        ) 'Run native sanitizer tests'
    }

    if (-not $SkipAndroid) {
        $java17 = Resolve-Java17Home
        if (-not $java17) {
            throw 'Android verification requires a JDK 17 installation.'
        }
        if (-not $androidSdk) {
            throw 'Android verification requires ANDROID_HOME, ANDROID_SDK_ROOT, or local.properties.'
        }

        $env:JAVA_HOME = $java17
        $env:ANDROID_HOME = $androidSdk
        $env:ANDROID_SDK_ROOT = $androidSdk
        $env:PATH = "$(Join-Path $java17 'bin');$env:PATH"

        $gradleTasks = @(
            '--no-daemon',
            ':usbHostForAndroid:test',
            ':usbHostForAndroid:lint',
            ':usbHostForAndroid:assembleDebug',
            ':usbHostForAndroid:assembleRelease',
            ':rtlSdrForAndroid:lint',
            ':rtlSdrForAndroid:assembleDebug'
        )
        if (-not $SkipPublication) {
            $gradleTasks += ':usbHostForAndroid:verifyReleasePublication'
        }
        Invoke-Checked (Join-Path $repo 'gradlew.bat') $gradleTasks `
            'Run managed tests, Android lint, assembly, and publication checks'

        if (-not $SkipPublication) {
            Invoke-Checked 'pwsh' @(
                '-NoProfile', '-File', (Join-Path $repo 'scripts\verify-publication.ps1'),
                '-RepositoryRoot', $repo
            ) 'Verify published native symbols'
            Invoke-Checked (Join-Path $repo 'gradlew.bat') @(
                '--no-daemon',
                '-p', (Join-Path $repo 'smoke-tests\android-consumer'),
                ':consumer:testDebugUnitTest',
                ':consumer:assembleDebug'
            ) 'Build detached Android consumer'
        }
    }

    Invoke-Checked 'pwsh' @(
        '-NoProfile',
        '-File', (Join-Path $repo 'scripts\verify-release.ps1')
    ) 'Verify release policy'

    if (-not $SkipGitHub) {
        Invoke-Checked 'pwsh' @(
            '-NoProfile',
            '-File', (Join-Path $repo 'scripts\verify-spec-task-issues.ps1'),
            '-Repository', $Repository
        ) 'Verify task-to-issue mapping'

        $actionsJson = @(& gh api "repos/$Repository/actions/permissions" 2>&1)
        if ($LASTEXITCODE -ne 0) {
            throw 'Unable to read GitHub Actions repository permission.'
        }
        $actions = ConvertFrom-Json ($actionsJson -join [Environment]::NewLine)
        if ([bool]$actions.enabled) {
            throw 'GitHub Actions must remain disabled until the maintainer explicitly restores them.'
        }
        Write-Host 'GitHub Actions repository execution is disabled.'
    }

    Assert-PublicPolicy
    Write-Host "`nAll requested local verification gates passed. No publication or hardware operation was performed."
    # The public-content scan ends on `git grep`, which exits 1 when it finds nothing. Without an
    # explicit success code that expected no-match would leak out as a failed verification run.
    exit 0
} finally {
    Pop-Location
}
