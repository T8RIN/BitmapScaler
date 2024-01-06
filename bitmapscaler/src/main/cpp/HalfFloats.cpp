/*
 * MIT License
 *
 * Copyright (c) 2023 Radzivon Bartoshyk
 * jxl-coder [https://github.com/awxkee/jxl-coder]
 *
 * Created by Radzivon Bartoshyk on 04/09/2023
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "HalfFloats.h"
#include <cstdint>
#include <__threading_support>
#include <vector>
#include <thread>
#include "half.hpp"

using namespace std;

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "HalfFloats.cpp"

#include "hwy/foreach_target.h"
#include "hwy/highway.h"

using namespace half_float;

uint as_uint(const float x) {
    return *(uint *) &x;
}

float as_float(const uint x) {
    return *(float *) &x;
}

uint16_t float_to_half(
        const float x) { // IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
    const uint b =
            as_uint(x) + 0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
    const uint e = (b & 0x7F800000) >> 23; // exponent
    const uint m = b &
                   0x007FFFFF; // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding
    return (b & 0x80000000) >> 16 | (e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) |
           ((e < 113) & (e > 101)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) |
           (e > 143) * 0x7FFF; // sign : normalized : denormalized : saturate
}

float half_to_float(
        const uint16_t x) { // IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
//    const uint e = (x & 0x7C00) >> 10; // exponent
//    const uint m = (x & 0x03FF) << 13; // mantissa
//    const uint v = as_uint((float) m)
//            >> 23; // evil log2 bit hack to count leading zeros in denormalized format
//    return as_float((x & 0x8000) << 16 | (e != 0) * ((e + 112) << 23 | m) | ((e == 0) & (m != 0)) *
//                                                                            ((v - 37) << 23 |
//                                                                             ((m << (150 - v)) &
//                                                                              0x007FE000))); // sign : normalized : denormalized
    half f;
    f.data_ = x;
    return f;
}

HWY_BEFORE_NAMESPACE();

namespace coder::HWY_NAMESPACE {

    using hwy::HWY_NAMESPACE::FixedTag;
    using hwy::HWY_NAMESPACE::Vec;
    using hwy::HWY_NAMESPACE::LoadInterleaved4;
    using hwy::HWY_NAMESPACE::Rebind;
    using hwy::HWY_NAMESPACE::Store;
    using hwy::HWY_NAMESPACE::BitCast;
    using hwy::HWY_NAMESPACE::StoreInterleaved4;

    void RGBAF32ToF16RowHWY(const float *HWY_RESTRICT src, uint16_t *dst, int width) {
        const FixedTag<float, 4> df32;
        const FixedTag<hwy::float16_t, 4> df16;
        const FixedTag<uint16_t, 4> du16;
        using V32 = Vec<decltype(df32)>;
        using V16 = Vec<decltype(df16)>;
        const Rebind<hwy::float16_t, FixedTag<float, 4>> dfc16;
        int x = 0;
        int pixelsCount = 4;
        for (x = 0; x + pixelsCount < width; x += pixelsCount) {
            V32 pixels1;
            V32 pixels2;
            V32 pixels3;
            V32 pixels4;
            LoadInterleaved4(df32, src, pixels1, pixels2, pixels3, pixels4);
            auto pixeld1 = BitCast(du16, DemoteTo(dfc16, pixels1));
            auto pixeld2 = BitCast(du16, DemoteTo(dfc16, pixels2));
            auto pixeld3 = BitCast(du16, DemoteTo(dfc16, pixels3));
            auto pixeld4 = BitCast(du16, DemoteTo(dfc16, pixels4));
            StoreInterleaved4(pixeld1, pixeld2, pixeld3, pixeld4, du16,
                              reinterpret_cast<uint16_t *>(dst));
            src += 4 * pixelsCount;
            dst += 4 * pixelsCount;
        }

        for (; x < width; ++x) {
            dst[0] = half(src[0]).data_;
            dst[1] = half(src[1]).data_;
            dst[2] = half(src[2]).data_;
            dst[3] = half(src[3]).data_;

            src += 4;
            dst += 4;
        }
    }

    void
    RgbaF32ToF16H(const float *HWY_RESTRICT src, int srcStride, uint16_t *HWY_RESTRICT dst,
                  int dstStride, int width,
                  int height) {
        auto srcPixels = reinterpret_cast<const uint8_t *>(src);
        auto dstPixels = reinterpret_cast<uint8_t *>(dst);

        int threadCount = clamp(min(static_cast<int>(std::thread::hardware_concurrency()),
                                    width * height / (256 * 256)), 1, 12);
        std::vector<std::thread> workers;

        int segmentHeight = height / threadCount;

        for (int i = 0; i < threadCount; i++) {
            int start = i * segmentHeight;
            int end = (i + 1) * segmentHeight;
            if (i == threadCount - 1) {
                end = height;
            }
            workers.emplace_back(
                    [start, end, srcPixels, dstPixels, srcStride, dstStride, width]() {
                        for (int y = start; y < end; ++y) {
                            RGBAF32ToF16RowHWY(
                                    reinterpret_cast<const float *>(srcPixels + srcStride * y),
                                    reinterpret_cast<uint16_t *>(dstPixels + dstStride * y),
                                    width);
                        }
                    });
        }

        for (std::thread &thread: workers) {
            thread.join();
        }
    }
}

HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace coder {
    HWY_EXPORT(RgbaF32ToF16H);
    HWY_DLLEXPORT void
    RgbaF32ToF16(const float *HWY_RESTRICT src, int srcStride, uint16_t *HWY_RESTRICT dst,
                 int dstStride, int width,
                 int height) {
        HWY_DYNAMIC_DISPATCH(RgbaF32ToF16H)(src, srcStride, dst, dstStride, width, height);
    }
}

#endif