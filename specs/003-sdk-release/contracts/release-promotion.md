# Contract: Release Promotion

## Branch topology

```text
main ──► dev ──► task branch ──PR──► dev ──approved PR──► main ──tag──► release
```

- `dev` is created from `main` and is the integration target for task branches.
- Every task branch is created from current `dev` and returns through a reviewed pull request.
- `main` is release-only. Direct feature commits and direct publication from `dev` are rejected.
- Promotion from `dev` to `main` requires explicit maintainer approval after candidate testing.

## Release gate

For version 0.1.0, automation MUST verify all of the following before upload:

1. Ref is exactly `refs/tags/v0.1.0` or an approved manual dry-run.
2. Tag commit is reachable from `origin/main`.
3. Gradle/POM version is exactly `0.1.0`.
4. Git submodules match committed revisions.
5. Unit, native, lint, Android assembly, sample, and publication-contract checks pass.
6. Central and signing secrets exist in the protected `release` environment.
7. Generated artifacts are attached to provenance evidence.

The current implementation task ends after merge to `dev`. It MUST NOT perform the approved PR,
tagging, release creation, or Central upload without a later explicit instruction.
