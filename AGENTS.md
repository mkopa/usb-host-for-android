# Public repository policy

- This repository is public at `github.com/mkopa/usb-host-for-android`.
- Every commit created in this repository must use both author and committer identity
  `Marci Kopa <marcin@marcin.info>`.
- Do not include private company, customer, or employer names in tracked files, commit messages,
  branch names, tags, examples, metadata, generated artifacts, or documentation.
- Keep examples generic and sanitize local paths, device identifiers, and hardware evidence before
  committing.
- Describe example hardware as an `STM32G0B0RET6 demonstration board` unless a public third-party
  development kit is identified precisely and supported by evidence.
- Before committing, inspect the staged diff and commit metadata for accidental private branding or
  identities.

# Temporary local-verification policy

- GitHub Actions are intentionally disabled for this repository while hosted execution is unavailable.
- Do not enable or dispatch GitHub Actions unless the maintainer explicitly asks to restore them.
- Keep existing workflow files intact for later reactivation, but do not use them as merge gates.
- Run all relevant native, Android, publication, and policy checks locally and record the exact
  commands and results in the linked issue and pull request before merging into `dev`.
