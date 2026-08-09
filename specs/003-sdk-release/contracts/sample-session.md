# Contract: Kotlin Sample Probe Session

## Supported user actions

- Scan attached compatible probes.
- Select one probe when more than one is present.
- Request Android USB permission through the system prompt.
- Open a session, display programmer information, and connect to the target.
- Read and format a 256-byte preview from the reported flash base.
- Disconnect and clear session-derived state.

## Prohibited user actions

The sample provides no write, erase, mass erase, option-byte, reset, halt, run, step, trace, target
register write, target memory write, or peripheral mutation action.

## Lifecycle and threading

- USB permission callbacks are registered only while the activity is active and use a package-scoped
  explicit broadcast intent.
- Open/connect/read/close work executes outside the Android main thread.
- One operation owns the session at a time; repeat taps do not create concurrent native calls.
- Detach, cancellation, failure, or activity destruction closes the session idempotently.
- The app never displays or logs USB serial numbers by default.

## Presentation

- Empty, permission, connecting, connected, reading, error, and detached states are distinguishable.
- Probe and target facts are labeled in human-readable form with hexadecimal values where useful.
- Memory preview uses address, hexadecimal bytes, and printable ASCII columns.
- Errors preserve an actionable status name/message without exposing sensitive device data.
