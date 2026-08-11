# GitHub Task Delivery Contract

## Scope

Every checkbox task generated for this feature maps to exactly one GitHub issue, one implementation
branch, and one pull request into `dev`. Planning artifacts themselves are not implementation tasks
unless a generated task explicitly names them.

## Permission preflight

Before creating any issue or branch:

1. Confirm repository issues are enabled.
2. Confirm the authenticated GitHub actor has `WRITE`, `MAINTAIN`, or `ADMIN`.
3. Confirm local `dev` matches `origin/dev` and the worktree contains no unrelated mutation.
4. Configure author and committer as `Marci Kopa <marcin@marcin.info>`.
5. Confirm GitHub Actions remain disabled and select the task's required local verification commands.

Read-only authentication blocks issue/PR/merge actions. It must not be bypassed by embedding tokens or
credentials in tracked files.

## Task-to-issue synchronization

Each task issue contains:

- title: `[004][TNNN] <task summary>`
- hidden idempotency marker: `<!-- speckit-task:004-generic-usb-transport:TNNN -->`
- user story, phase, exact file scope, dependencies, acceptance/verification commands, and safety
  constraints copied from the approved artifacts
- labels for feature, story/phase, and status when available

Synchronization is idempotent: an existing marker updates that issue instead of creating a duplicate.
A task split creates a new task ID and issue before implementation begins.

## Branch contract

```text
feat/<issue-number>-tNNN-<sanitized-kebab-slug>
```

- Branch from the latest merged `dev`, never from `main` or a stale task branch.
- Public-safe lowercase ASCII slug; no organization, customer, device identifier, or local path.
- One branch implements only its linked task. Dependencies must already be merged unless the task is
  explicitly independent and file-disjoint.

## Issue updates

The issue is updated when:

1. work starts (branch link and dependency state),
2. a PR opens (PR link and validation plan),
3. a local check fails or scope changes (concise diagnosis and next action),
4. all required local checks pass and merge begins,
5. merge completes (merge commit and verification result).

Updates contain sanitized evidence and no raw hardware identifier or private build log.

## Pull request contract

- Base branch: `dev`.
- Title includes `[004][TNNN]` and a public-safe summary.
- Body includes `Closes #<issue>`, scope, dependencies, exact checks run, API/ABI impact, hardware
  evidence status, and public-content audit.
- Author/committer metadata is verified before push.
- The staged diff is inspected for private branding, local paths, identifiers, generated binaries,
  unexpected dependencies, and unrelated user changes.

## Merge contract

The assistant merges a task PR only when:

- the branch is current with `dev`,
- all required native, Android, publication, and feature-specific checks pass locally,
- exact local commands and summarized results are recorded in the issue and PR,
- no unresolved review thread or requested change remains,
- public policy and commit identity checks pass,
- API/ABI baseline is unchanged unless the task's approved additive contract requires it,
- the linked issue accurately reflects the result.

GitHub Actions are intentionally disabled and are not a merge gate. The assistant performs the merge
after observing the local-verification gates, verifies issue closure, and deletes the remote task
branch when permitted. `main` remains untouched until a separately approved release promotion.
