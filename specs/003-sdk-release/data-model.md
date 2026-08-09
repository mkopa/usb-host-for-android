# Phase 1 Data Model: Public SDK Release 0.1.0

## LibraryRelease

Represents one immutable public SDK version.

| Field | Type | Rules |
|---|---|---|
| groupId | string | Exactly `info.marcin.usbhost` |
| artifactId | string | Exactly `usb-host-for-android` |
| version | semantic version | Exactly `0.1.0`; no snapshot suffix for release |
| namespace | Java package | Exactly `info.marcin.usbhost` |
| sourceCommit | SHA-1 | Must be contained in `main` before publication |
| artifacts | set | AAR, POM, module metadata, sources JAR, documentation JAR |
| signatures | set | Detached signature for every Central-required artifact |
| state | enum | `prepared`, `verified`, `approved`, `uploaded`, `published`, `failed` |

### State transitions

`prepared → verified → approved → uploaded → published`

Any validation or upload error transitions the candidate to `failed`. `failed` and `published` are
terminal for a given coordinate/version because Maven Central releases are immutable.

## BuildVerification

Represents evidence from one required quality gate.

| Field | Type | Rules |
|---|---|---|
| name | string | Stable workflow/job/task name |
| sourceCommit | SHA-1 | Same commit as candidate release |
| environment | string | Hosted runner or pinned container digest |
| command | string | Non-interactive and reproducible |
| result | enum | `pending`, `passed`, `failed`, `skipped` |
| evidence | URI/path | Log, report, artifact, or attestation |
| releaseRequired | boolean | Required gates cannot be skipped for publication |

## ProbeSessionUiState

The Kotlin sample's immutable screen state.

| Field | Type | Rules |
|---|---|---|
| devices | list of ProbeSummary | May be empty; stable selection key uses USB device ID |
| selectedDeviceId | integer? | Must refer to an attached compatible probe |
| permissionState | enum | `unknown`, `requesting`, `granted`, `denied` |
| connectionState | enum | `idle`, `opening`, `connected`, `reading`, `closing`, `error` |
| programmerInfo | ProgrammerInfo? | Present only after native session opens |
| targetInfo | TargetInfo? | Present only after successful target connection |
| memoryPreview | ByteArray? | Exactly requested bounded preview; cleared on session change |
| logEntries | bounded list | User-safe messages, newest first, no device serial by default |
| error | display error? | Stable title plus actionable context; no raw secrets/identifiers |

### State transitions

- `idle → requesting → opening → connected`
- `connected → reading → connected`
- Any active state → `closing → idle`
- Permission denial returns to `idle` with a non-fatal message.
- USB detach closes the session and returns to `idle`; prior target/memory values are cleared.
- Operation failure enters `error`, closes native resources, then permits scan/retry.

## ReleasePromotion

Represents controlled movement of a tested candidate.

| Field | Type | Rules |
|---|---|---|
| sourceBranch | string | `dev` |
| destinationBranch | string | `main` |
| candidateCommit | SHA-1 | Head reviewed and tested on `dev` |
| approval | identity + timestamp | Explicit maintainer approval required |
| checks | list | All release-required BuildVerification records passed |
| tag | string | `v` plus LibraryRelease.version |
| outcome | enum | `pending`, `merged`, `tagged`, `published`, `rejected` |

Promotion cannot advance from `pending` without approval and passed checks. Tagging cannot precede
merge to `main`; publication cannot precede tag verification.
