FROM eclipse-temurin:17-jdk-jammy

ARG ANDROID_COMMAND_LINE_TOOLS=13114758
ARG ANDROID_API=37
ARG ANDROID_BUILD_TOOLS=37.0.0
ARG ANDROID_NDK=28.2.13676358
ARG ANDROID_CMAKE=3.22.1

ENV ANDROID_HOME=/opt/android-sdk \
    ANDROID_SDK_ROOT=/opt/android-sdk \
    GRADLE_USER_HOME=/opt/gradle-cache \
    PATH=/opt/android-sdk/cmdline-tools/latest/bin:/opt/android-sdk/platform-tools:$PATH

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates curl git ninja-build unzip \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir -p "$ANDROID_HOME/cmdline-tools" "$GRADLE_USER_HOME" \
    && curl --fail --location --retry 3 \
        "https://dl.google.com/android/repository/commandlinetools-linux-${ANDROID_COMMAND_LINE_TOOLS}_latest.zip" \
        -o /tmp/android-tools.zip \
    && unzip -q /tmp/android-tools.zip -d /tmp/android-tools \
    && mv /tmp/android-tools/cmdline-tools "$ANDROID_HOME/cmdline-tools/latest" \
    && rm -rf /tmp/android-tools /tmp/android-tools.zip \
    && yes | sdkmanager --licenses >/dev/null \
    && sdkmanager \
        "platform-tools" \
        "platforms;android-35" \
        "platforms;android-${ANDROID_API}" \
        "build-tools;${ANDROID_BUILD_TOOLS}" \
        "ndk;${ANDROID_NDK}" \
        "cmake;${ANDROID_CMAKE}" \
    && chmod -R a+rX "$ANDROID_HOME" "$GRADLE_USER_HOME"

WORKDIR /workspace

COPY smoke-test.sh /usr/local/bin/android-runner-smoke-test
RUN chmod +x /usr/local/bin/android-runner-smoke-test \
    && android-runner-smoke-test

CMD ["./gradlew", "--no-daemon", "test", "lint", "assembleRelease"]
