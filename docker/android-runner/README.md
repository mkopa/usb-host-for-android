# Pinned Android build runner

The image contains JDK 17, Android platforms 35 and 37, build-tools 37.0.0, NDK
28.2.13676358, CMake 3.22.1, Ninja, Git, and Android command-line tools. It is suitable for the
repository's Android, JNI, unit, lint, and publication-contract builds.

Build locally:

```powershell
docker build -t usb-host-android-runner docker/android-runner
docker run --rm -v "${PWD}:/workspace" -w /workspace usb-host-android-runner
```

After the runner workflow completes, pull the public image:

```powershell
docker pull ghcr.io/mkopa/usb-host-android-runner:latest
```

GitHub Container Registry is the default because this public repository can publish there with its
built-in token. To mirror the same immutable image to Docker Hub, configure repository secrets
`DOCKERHUB_USERNAME` and `DOCKERHUB_TOKEN`; no registry credential belongs in source control.
