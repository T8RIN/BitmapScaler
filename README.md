# Bitmap Scaler for Android API 14+

This BitmapScaler with different scaling algorhytms based on jxl-coder from awxkee can upscale or downscale bitmaps with 8 options:
* Bilinear
* Nearest Neighbour
* Bicubic
* Mitchell
* Lanczos
* Catmull
* Hermite
* Spline
* Hann


# Add Jitpack repository

```kotlin
repositories {
    maven { setUrl("https://jitpack.io") }
}
```

```kotlin
implementation("com.github.t8rin:bitmap-scaler:1.1.1")
```

# Self-build

## Requirements

- ndk
- ninja
- cmake
- nasm

**All commands are require the NDK path set by NDK_PATH environment variable**

* If you wish to build for **x86** you have to add a **$INCLUDE_X86** environment variable for
  example:*

```shell
NDK_PATH=/path/to/ndk INCLUDE_X86=yes bash build_jxl.sh
```

# Copyrights

This library created with [jxl-coder](https://github.com/awxkee/jxl-coder) sources
