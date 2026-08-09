# Contract: Maven Publication 0.1.0

## Consumer coordinates

```text
Repository: Maven Central
Group:      info.marcin.usbhost
Artifact:   usb-host-for-android
Version:    0.1.0
Namespace:  info.marcin.usbhost
Packaging:  aar
```

Gradle consumers use:

```kotlin
repositories { mavenCentral() }
dependencies { implementation("info.marcin.usbhost:usb-host-for-android:0.1.0") }
```

## Required release bundle

- Release AAR containing managed classes, Android manifest, supported ABI libraries, and Prefab.
- POM with project name/description/URL, MIT license, developer, SCM connection, and issue tracker.
- Gradle module metadata.
- Sources JAR and non-empty documentation JAR.
- Detached OpenPGP signatures and checksums required by Central for all bundle members.

## Validation invariants

- No public class, manifest namespace, JNI symbol, consumer keep rule, sample import, or test package
  references the previous development namespace.
- Version has no `SNAPSHOT`, build number, or timestamp suffix.
- Publication dry-run writes to an isolated local Maven repository and performs no network upload.
- A live publish requires Central username/password and in-memory signing key/password supplied only
  by the protected release environment.
- A duplicate immutable version is never overwritten or silently retried as different bytes.
