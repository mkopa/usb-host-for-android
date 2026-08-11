[CmdletBinding()]
param(
    [string]$FeatureId = '004-generic-usb-transport',
    [string]$TasksPath = (Join-Path $PSScriptRoot '..\specs\004-generic-usb-transport\tasks.md'),
    [string]$Repository = 'mkopa/usb-host-for-android',
    [string]$IssuesJson
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Add-VerificationError {
    param(
        [System.Collections.Generic.List[string]]$Errors,
        [string]$Message
    )

    $Errors.Add($Message)
}

if (-not (Test-Path -LiteralPath $TasksPath -PathType Leaf)) {
    throw "Tasks file does not exist: $TasksPath"
}

$taskMatches = @(
    Get-Content -LiteralPath $TasksPath |
        ForEach-Object {
            if ($_ -match '^- \[(?: |x|X)\]\s+(T\d{3})\b') {
                $Matches[1]
            }
        }
)

if ($taskMatches.Count -eq 0) {
    throw "No checkbox tasks were found in: $TasksPath"
}

$errors = [System.Collections.Generic.List[string]]::new()
$taskGroups = @($taskMatches | Group-Object)
foreach ($group in $taskGroups) {
    if ($group.Count -ne 1) {
        Add-VerificationError $errors "Task $($group.Name) occurs $($group.Count) times in tasks.md."
    }
}

$taskIds = @($taskGroups | ForEach-Object Name | Sort-Object)
$taskSet = @{}
foreach ($taskId in $taskIds) {
    $taskSet[$taskId] = $true
}

if ([string]::IsNullOrWhiteSpace($IssuesJson)) {
    if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
        throw 'GitHub CLI (gh) is required when -IssuesJson is not provided.'
    }

    $rawIssues = @(
        & gh issue list --repo $Repository --state all --limit 1000 `
            --json number,title,body,state,url 2>&1
    )
    if ($LASTEXITCODE -ne 0) {
        throw "Unable to list GitHub issues for $Repository. Check gh authentication and access."
    }
    $IssuesJson = $rawIssues -join [Environment]::NewLine
}

try {
    $issues = @(ConvertFrom-Json -InputObject $IssuesJson)
} catch {
    throw 'Issue input is not valid JSON.'
}

$markerPattern = '<!--\s*speckit-task:(?<feature>[a-zA-Z0-9._-]+):(?<task>T\d{3})\s*-->'
$taskIssues = @{}
foreach ($taskId in $taskIds) {
    $taskIssues[$taskId] = [System.Collections.Generic.List[object]]::new()
}

foreach ($issue in $issues) {
    $number = [int]$issue.number
    $body = if ($null -eq $issue.body) { '' } else { [string]$issue.body }
    $markers = @([regex]::Matches($body, $markerPattern))
    $featureMarkers = @(
        $markers | Where-Object { $_.Groups['feature'].Value -eq $FeatureId }
    )

    if ($body -match '<!--\s*speckit-task:' -and $markers.Count -eq 0) {
        Add-VerificationError $errors "Issue #$number contains a malformed task marker."
    }

    if ($featureMarkers.Count -gt 1) {
        Add-VerificationError $errors "Issue #$number contains $($featureMarkers.Count) markers for feature $FeatureId."
    }

    foreach ($marker in $featureMarkers) {
        $taskId = $marker.Groups['task'].Value
        if (-not $taskSet.ContainsKey($taskId)) {
            Add-VerificationError $errors "Issue #$number maps orphan marker $FeatureId/$taskId."
            continue
        }
        $taskIssues[$taskId].Add($issue)
    }
}

foreach ($taskId in $taskIds) {
    $mapped = $taskIssues[$taskId]
    if ($mapped.Count -eq 0) {
        Add-VerificationError $errors "Task $taskId has no GitHub issue marker."
    } elseif ($mapped.Count -gt 1) {
        $numbers = @($mapped | ForEach-Object { "#$([int]$_.number)" }) -join ', '
        Add-VerificationError $errors "Task $taskId maps to $($mapped.Count) issues: $numbers."
    }
}

if ($errors.Count -gt 0) {
    $errors | Sort-Object -Unique | ForEach-Object { Write-Error $_ }
    throw "Task-to-issue verification failed with $($errors.Count) finding(s)."
}

Write-Output "Verified $($taskIds.Count) unique tasks and exactly one GitHub issue marker per task for $FeatureId."
