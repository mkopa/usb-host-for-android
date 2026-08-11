# Release process

The repository uses a development-first flow:

```text
main → dev → task branch → pull request to dev → approved promotion to main → version tag
```

- Create every task branch from current `dev` and merge it back through a pull request.
- Treat `main` as release-only. Promotion from `dev` requires explicit maintainer approval.
- A release tag must be `v` followed by the exact `VERSION_NAME` from `gradle.properties`.
- Release automation verifies that the tagged commit belongs to `main` before publishing.

## Maven Central setup

The public coordinate is:

```text
info.marcin.usbhost:usb-host-for-android:0.1.0
```

Maven Central is the canonical repository. Google Maven is a dependency source for Android platform
artifacts, not the publication destination for this third-party AAR. The Gradle Plugin Portal is for
Gradle plugins and is not applicable to this library.

Before the first release, the maintainer must register and verify `info.marcin` in the Sonatype
Central Portal, publish the OpenPGP public key, create a Central user token, and configure the
protected GitHub `release` environment with:

| Secret | Purpose |
|---|---|
| `MAVEN_CENTRAL_USERNAME` | Central Portal token username |
| `MAVEN_CENTRAL_PASSWORD` | Central Portal token password |
| `SIGNING_IN_MEMORY_KEY` | ASCII-armored OpenPGP private key |
| `SIGNING_IN_MEMORY_KEY_ID` | Optional short key identifier |
| `SIGNING_IN_MEMORY_KEY_PASSWORD` | Private-key password |

Never put these values in `gradle.properties`, workflow YAML, build logs, or repository files.

## Candidate verification

Use JDK 17. AGP's documentation generator is not supported by this project on JDK 25.

```powershell
$env:JAVA_HOME='C:\Program Files\Java\jdk-17'
./gradlew.bat --no-daemon clean test lint assembleDebug assembleRelease
./gradlew.bat --no-daemon :usbHostForAndroid:verifyReleasePublication
./gradlew.bat --no-daemon -p smoke-tests/android-consumer assembleDebug
cmake -S native-tests -B build/native-tests
cmake --build build/native-tests --config Debug
ctest --test-dir build/native-tests -C Debug --output-on-failure
```

`verifyReleasePublication` writes and inspects an unsigned local candidate under
`usbHostForAndroid/build/repository`; it never uploads to Central.

## Promotion and publication

1. Confirm every required task and CI check is complete on `dev`.
2. Open and approve the `dev` to `main` pull request.
3. Confirm `main` still has `VERSION_NAME=0.1.0` and a clean working tree.
4. Create and push the signed tag `v0.1.0` from the approved `main` commit.
5. The release workflow rebuilds and tests, signs the bundle, publishes through the current Central
   Portal API, creates build provenance, and attaches artifacts to the GitHub release.
6. Central synchronization normally takes additional time; confirm the coordinate resolves before
   announcing availability.

Release bytes are immutable. Never reuse or overwrite an existing Maven Central version.
