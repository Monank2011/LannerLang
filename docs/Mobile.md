# Lanner Mobile

Lanner can target Android and iOS as native ARM code while keeping the application core in Lanner and using the platform's native application boundary for UI and device integration.

## Compiler targets

Android ABIs:

- `arm64-v8a` -> `aarch64-linux-android<api>`
- `armeabi-v7a` -> `armv7a-linux-androideabi<api>`
- `x86_64` -> `x86_64-linux-android<api>`
- `x86` -> `i686-linux-android<api>`

Apple:

- iOS device -> `arm64-apple-ios<deployment>`
- iOS Simulator -> `arm64-apple-ios<deployment>-simulator`

Examples:

```text
lanner examples/mobile_app.st --target=arm64-apple-ios16.0 --emit-object -o app.o
lanner examples/mobile_app.st --target=arm64-apple-ios16.0-simulator --emit-object -o app-sim.o
lanner examples/mobile_app.st --target=aarch64-linux-android24 --emit-object -o app.o
```

## Project generation

Android:

```text
lanner examples/mobile_app.st --android-project MyAndroidApp \
  --android-abi arm64-v8a --deployment 24 \
  --mobile-name LannerMobile --bundle-id com.example.lannermobile
```

The generated project contains a Gradle/NDK/CMake native library, JNI bridge, Lanner source, and a native Activity UI. Android's official NDK workflow supports packaging native libraries built with CMake and using JNI for application-layer interoperation.

iOS:

```text
lanner examples/mobile_app.st --ios-project MyIOSApp \
  --deployment 16.0 --mobile-name LannerMobile \
  --bundle-id com.example.lannermobile
```

Add `--ios-simulator` to target arm64 Simulator code. The generated Xcode project contains SwiftUI application UI, an Objective-C bridge, the Lanner native object build phase, and platform framework linkage.

## Mobile namespace

Lanner's core mobile API is intentionally small and platform-neutral:

```text
Mobile.log(message)
Mobile.platform()
Mobile.osVersion()
Mobile.isSimulator()
Mobile.screenWidth()
Mobile.screenHeight()
Mobile.deviceScale()
Mobile.safeAreaTop()
Mobile.safeAreaBottom()
Mobile.safeAreaLeft()
Mobile.safeAreaRight()
Mobile.appDataPath()
Mobile.documentsPath()
Mobile.cachePath()
Mobile.openUrl(url)
Mobile.vibrate(milliseconds)
Mobile.requestPermission(kind)
Mobile.clipboardSet(text)
Mobile.clipboardGet()
Mobile.cameraAvailable()
Mobile.locationAvailable()
Mobile.bluetoothAvailable()
```

`clipboardGet()` returns an owned `Buffer` and must be released with `Buffer.free()`.

## Architecture

Lanner does not replace Android's JVM or Apple's Objective-C/Swift UI framework. Instead, the generated project makes the interop boundary explicit:

```text
Lanner native core
       |
       +-- Android: C/JNI -> Kotlin Activity -> Android APIs
       |
       +-- iOS: C/Objective-C -> SwiftUI -> Apple frameworks
```

This keeps performance-sensitive code native while allowing platform-native UI and device facilities.
