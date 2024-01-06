# Bitmap Scaler for Android API 14+

This BitmapScaler with different scaling algorhytms based on jxl-coder from awxkee can upscale or downscale bitmaps with 8 options:
* Bilinear
* Nearest
* Cubic
* Mitchell
* Lanczos
* CatmullRom
* Hermite
* BSpline
* Hann


# Add Jitpack repository

```groovy
repositories {
    maven { url "https://jitpack.io" }
}
```

```groovy
implementation 'com.t8rin.bitmapscaler:bitmapscaler:1.0.0' // or any version above picker from release tags
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
