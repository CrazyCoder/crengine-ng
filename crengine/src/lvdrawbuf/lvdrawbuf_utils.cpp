/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007,2010-2013 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2018 Aleksey Chernov <valexlin@gmail.com>               *
 *   Copyright (C) 2019 NiLuJe <ninuje@gmail.com>                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#include "lvdrawbuf_utils.h"
#include <lvdrawbuf.h>

// clang-format off

#if 0
static const short dither_2bpp_4x4[] = {
     5, 13,  8, 16,
     9,  1, 12,  4,
     7, 15,  6, 14,
    11,  3, 10,  2,
};
#endif

static const short dither_2bpp_8x8[] = {
     0, 32, 12, 44,  2, 34, 14, 46,
    48, 16, 60, 28, 50, 18, 62, 30,
     8, 40,  4, 36, 10, 42,  6, 38,
    56, 24, 52, 20, 58, 26, 54, 22,
     3, 35, 15, 47,  1, 33, 13, 45,
    51, 19, 63, 31, 49, 17, 61, 29,
    11, 43,  7, 39,  9, 41,  5, 37,
    59, 27, 55, 23, 57, 25, 53, 21,
};

// clang-format on

lUInt32 Dither1BitColor(lUInt32 color, lUInt32 x, lUInt32 y) {
    int cl = ((((color >> 16) & 255) + ((color >> 8) & 255) + ((color) & 255)) * (256 / 3)) >> 8;
    if (cl < 16)
        return 0;
    else if (cl >= 240)
        return 1;
    //int d = dither_2bpp_4x4[(x&3) | ( (y&3) << 2 )] - 1;
    int d = dither_2bpp_8x8[(x & 7) | ((y & 7) << 3)] - 1;

    cl = (cl + d - 32);
    if (cl < 5)
        return 0;
    else if (cl >= 250)
        return 1;
    return (cl >> 7) & 1;
}

lUInt32 Dither2BitColor(lUInt32 color, lUInt32 x, lUInt32 y) {
    int cl = ((((color >> 16) & 255) + ((color >> 8) & 255) + ((color) & 255)) * (256 / 3)) >> 8;
    if (cl < 5)
        return 0;
    else if (cl >= 250)
        return 3;
    //int d = dither_2bpp_4x4[(x&3) | ( (y&3) << 2 )] - 1;
    int d = dither_2bpp_8x8[(x & 7) | ((y & 7) << 3)] - 1;

    cl = (cl + d - 32);
    if (cl < 5)
        return 0;
    else if (cl >= 250)
        return 3;
    return (cl >> 6) & 3;
}

// returns byte with higher significant bits, lower bits are 0
lUInt32 DitherNBitColor(lUInt32 color, lUInt32 x, lUInt32 y, int bits) {
    int mask = ((1 << bits) - 1) << (8 - bits);
    // gray = (r + 2*g + b)>>2
    //int cl = ((((color>>16) & 255) + ((color>>(8-1)) & (255<<1)) + ((color) & 255)) >> 2) & 255;
    int cl = ((((color >> 16) & 255) + ((color >> (8 - 1)) & (255 << 1)) + ((color) & 255)) >> 2) & 255;
    int white = (1 << bits) - 1;
    int precision = white;
    if (cl < precision)
        return 0;
    else if (cl >= 255 - precision)
        return mask;
    //int d = dither_2bpp_4x4[(x&3) | ( (y&3) << 2 )] - 1;
    // dither = 0..63
    int d = dither_2bpp_8x8[(x & 7) | ((y & 7) << 3)] - 1;
    int shift = bits - 2;
    cl = ((cl << shift) + d - 32) >> shift;
    if (cl > 255)
        cl = 255;
    if (cl < 0)
        cl = 0;
    return cl & mask;
}

// NOTE: For more accurate (but slightly more costly) conversions, see:
//       stb does (lUInt8) (((r*77) + (g*150) + (b*29)) >> 8) (That's roughly the Rec601Luma algo)
//       Qt5 does (lUInt8) (((r*11) + (g*16) + (b*5)) >> 5) (That's closer to Rec601Luminance or Rec709Luminance IIRC)
lUInt32 rgbToGray(lUInt32 color) {
    lUInt32 r = (0xFF0000 & color) >> 16;
    lUInt32 g = (0x00FF00 & color) >> 8;
    lUInt32 b = (0x0000FF & color) >> 0;
    return ((r + g + g + b) >> 2) & 0xFF;
}

lUInt8 rgbToGray(lUInt32 color, int bpp) {
    lUInt32 r = (0xFF0000 & color) >> 16;
    lUInt32 g = (0x00FF00 & color) >> 8;
    lUInt32 b = (0x0000FF & color) >> 0;
    return (lUInt8)(((r + g + g + b) >> 2) & (((1 << bpp) - 1) << (8 - bpp)));
}

lUInt8 rgbToGrayMask(lUInt32 color, int bpp) {
    switch (bpp) {
        case DRAW_BUF_1_BPP:
            color = rgbToGray(color) >> 7;
            color = (color & 1) ? 0xFF : 0x00;
            break;
        case DRAW_BUF_2_BPP:
            color = rgbToGray(color) >> 6;
            color &= 3;
            color |= (color << 2) | (color << 4) | (color << 6);
            break;
        case DRAW_BUF_3_BPP: // 8 colors
        case DRAW_BUF_4_BPP: // 16 colors
        case DRAW_BUF_8_BPP: // 256 colors
            // return 8 bit as is
            color = rgbToGray(color);
            color &= ((1 << bpp) - 1) << (8 - bpp);
            return (lUInt8)color;
        default:
            color = rgbToGray(color);
            return (lUInt8)color;
    }
    return (lUInt8)color;
}

void ApplyAlphaRGB(lUInt32& dst, lUInt32 src, lUInt32 alpha) {
    if (alpha == 0) {
        dst = src;
    } else if (alpha < 255) {
        src &= 0xFFFFFF;
        lUInt32 opaque = alpha ^ 0xFF;
        lUInt32 n1 = (((dst & 0xFF00FF) * alpha + (src & 0xFF00FF) * opaque) >> 8) & 0xFF00FF;
        lUInt32 n2 = (((dst & 0x00FF00) * alpha + (src & 0x00FF00) * opaque) >> 8) & 0x00FF00;
        dst = n1 | n2;
    }
}

void ApplyAlphaRGB565(lUInt16& dst, lUInt16 src, lUInt32 alpha) {
    if (alpha == 0) {
        dst = src;
    } else if (alpha < 255) {
        lUInt32 opaque = alpha ^ 0xFF;
        lUInt32 r = (((dst & 0xF800) * alpha + (src & 0xF800) * opaque) >> 8) & 0xF800;
        lUInt32 g = (((dst & 0x07E0) * alpha + (src & 0x07E0) * opaque) >> 8) & 0x07E0;
        lUInt32 b = (((dst & 0x001F) * alpha + (src & 0x001F) * opaque) >> 8) & 0x001F;
        dst = (lUInt16)(r | g | b);
    }
}

// obsoleted, ready to remove
void ApplyAlphaGray(lUInt8& dst, lUInt8 src, lUInt32 alpha, int bpp) {
    if (alpha == 0) {
        dst = src;
    } else if (alpha < 255) {
        int mask = ((1 << bpp) - 1) << (8 - bpp);
        src &= mask;
        lUInt32 opaque = alpha ^ 0xFF;
        lUInt32 n1 = ((dst * alpha + src * opaque) >> 8) & mask;
        dst = (lUInt8)n1;
    }
}

/*
void ApplyAlphaGray8( lUInt8 &dst, lUInt8 src, lUInt8 alpha )
{
    if ( alpha==0 ) {
        dst = src;
    } else if ( alpha < 255 ) {
        lUInt8 opaque = alpha ^ 0xFF;
        lUInt8 v = ((dst * alpha + src * opaque) >> 8);
        dst = (lUInt8) v;
    }
}
*/

#include <lvbasedrawbuf.h>
#include <crsetup.h>
#include <string.h>
#include <math.h>

DitheringOptions getDefault1BitDitheringOptions() {
    // Tuned for 1-bit e-ink: slightly higher threshold for darker output,
    // reduced error diffusion to minimize noise
    return DitheringOptions(0.70f, 0.85f, 1.0f, true);
}

DitheringOptions getDefault2BitDitheringOptions() {
    // 2-bit has more levels, needs less aggressive adjustments
    return DitheringOptions(0.5f, 0.95f, 1.0f, true);
}

void applyFloydSteinbergDither(LVBaseDrawBuf* dst, int dst_x, int dst_y,
                               lUInt8* grayBuf, int width, int height, int targetBpp,
                               const DitheringOptions* options) {
    if (!grayBuf || width <= 0 || height <= 0)
        return;

    // Use provided options or defaults based on targetBpp
    DitheringOptions opts;
    if (options) {
        opts = *options;
    } else {
        opts = (targetBpp == 1) ? getDefault1BitDitheringOptions() : getDefault2BitDitheringOptions();
    }

    // Clamp options to valid ranges
    if (opts.threshold < 0.0f) opts.threshold = 0.0f;
    if (opts.threshold > 1.0f) opts.threshold = 1.0f;
    if (opts.errorDiffusion < 0.0f) opts.errorDiffusion = 0.0f;
    if (opts.errorDiffusion > 1.0f) opts.errorDiffusion = 1.0f;
    if (opts.gamma < 0.1f) opts.gamma = 0.1f;
    if (opts.gamma > 3.0f) opts.gamma = 3.0f;

    // Build gamma lookup table (only if gamma != 1.0)
    lUInt8 gammaLUT[256];
    bool useGamma = (opts.gamma < 0.99f || opts.gamma > 1.01f);
    if (useGamma) {
        float invGamma = 1.0f / opts.gamma;
        for (int i = 0; i < 256; i++) {
            float normalized = i / 255.0f;
            float corrected = powf(normalized, invGamma);
            int val = (int)(corrected * 255.0f + 0.5f);
            gammaLUT[i] = (lUInt8)(val < 0 ? 0 : (val > 255 ? 255 : val));
        }
    }

    // Determine number of output levels
    const int numLevels = (1 << targetBpp);
    const int maxLevel = numLevels - 1;

    // Calculate threshold value (0-255 scale)
    const int threshold1bit = (int)(opts.threshold * 255.0f + 0.5f);

    // Error diffusion multiplier (fixed-point: 256 = 1.0)
    const int errMult = (int)(opts.errorDiffusion * 256.0f + 0.5f);

    // Quantize value to target level with configurable threshold
    auto quantize = [targetBpp, maxLevel, threshold1bit, &opts](int val8) -> int {
        if (val8 < 0) val8 = 0;
        if (val8 > 255) val8 = 255;
        int quantized;
        if (targetBpp == 1) {
            // Use configurable threshold for 1-bit
            quantized = (val8 >= threshold1bit) ? 255 : 0;
        } else {
            // For 2-bit, adjust quantization boundaries based on threshold
            // threshold=0.5 gives even spacing, other values shift boundaries
            float thresholdShift = (opts.threshold - 0.5f) * 2.0f; // -1.0 to +1.0
            int adjusted = val8 - (int)(thresholdShift * 42.5f);   // ~1/6 of 255
            if (adjusted < 0) adjusted = 0;
            if (adjusted > 255) adjusted = 255;
            int level = (adjusted * maxLevel + 128) / 256;
            if (level > maxLevel) level = maxLevel;
            quantized = level * 255 / maxLevel;
        }
        return quantized;
    };

    // Allocate error buffers for current and next row
    short* errorCur = new short[width + 2]();
    short* errorNext = new short[width + 2]();
    errorCur++;
    errorNext++;

    lvRect clip;
    dst->GetClipRect(&clip);
    int bpp = dst->GetBitsPerPixel();

    // Apply gamma correction to input buffer if needed
    if (useGamma) {
        for (int i = 0; i < width * height; i++) {
            grayBuf[i] = gammaLUT[grayBuf[i]];
        }
    }

    // Process each row with Floyd-Steinberg error diffusion
    for (int y = 0; y < height; y++) {
        memset(errorNext - 1, 0, (width + 2) * sizeof(short));
        lUInt8* srcRow = grayBuf + (y * width);

        // Serpentine: alternate direction each row
        bool leftToRight = !opts.serpentine || ((y & 1) == 0);
        int xStart = leftToRight ? 0 : (width - 1);
        int xEnd = leftToRight ? width : -1;
        int xStep = leftToRight ? 1 : -1;

        for (int x = xStart; x != xEnd; x += xStep) {
            // Get current pixel value plus accumulated error
            int oldVal = srcRow[x] + errorCur[x];

            // Quantize and calculate error
            int newVal = quantize(oldVal);
            int error = oldVal - newVal;

            // Scale error by diffusion strength (fixed-point math)
            error = (error * errMult) >> 8;

            // Store dithered value back to buffer for rendering
            srcRow[x] = (lUInt8)(newVal > 255 ? 255 : (newVal < 0 ? 0 : newVal));

            // Distribute error using Floyd-Steinberg weights
            // Direction depends on serpentine mode
            int xForward = leftToRight ? (x + 1) : (x - 1);
            int xBack = leftToRight ? (x - 1) : (x + 1);

            if (xForward >= 0 && xForward < width)
                errorCur[xForward] += (short)((error * 7) / 16);
            if (y + 1 < height) {
                if (xBack >= 0 && xBack < width)
                    errorNext[xBack] += (short)((error * 3) / 16);
                errorNext[x] += (short)((error * 5) / 16);
                if (xForward >= 0 && xForward < width)
                    errorNext[xForward] += (short)((error * 1) / 16);
            }
        }

        // Render this row to destination buffer
        int yy = y + dst_y;
        if (yy >= clip.top && yy < clip.bottom) {
            if (bpp == 2) {
                lUInt8* dstRow = (lUInt8*)dst->GetScanLine(yy);
                for (int x = 0; x < width; x++) {
                    int xx = x + dst_x;
                    if (xx < clip.left || xx >= clip.right)
                        continue;

                    lUInt32 dcl;
                    if (targetBpp == 1) {
                        // For F-S 1-bit, srcRow should contain only 0 or 255
                        // Force to black (0) or white (3) in 2-bit buffer
                        // This ensures no intermediate grays slip through
                        dcl = (srcRow[x] >= 128) ? 3 : 0;
                    } else {
                        // Convert 0-255 grayscale to 2-bit (0-3)
                        dcl = (srcRow[x] * 3 + 128) / 256;
                    }
#if (GRAY_INVERSE == 1)
                    dcl = 3 - dcl;
#endif
                    int byteindex = (xx >> 2);
                    int bitindex = (3 - (xx & 3)) << 1;
                    lUInt8 mask = 0xC0 >> (6 - bitindex);
                    dcl = dcl << bitindex;
                    dstRow[byteindex] = (lUInt8)((dstRow[byteindex] & (~mask)) | dcl);
                }
            } else if (bpp == 1) {
                lUInt8* dstRow = (lUInt8*)dst->GetScanLine(yy);
                for (int x = 0; x < width; x++) {
                    int xx = x + dst_x;
                    if (xx < clip.left || xx >= clip.right)
                        continue;

                    // Convert to 1-bit (threshold already applied by F-S)
                    lUInt32 dcl = srcRow[x] >= 128 ? 1 : 0;
#if (GRAY_INVERSE == 1)
                    dcl = 1 - dcl;
#endif
                    int byteindex = (xx >> 3);
                    int bitindex = ((xx & 7));
                    lUInt8 mask = 0x80 >> (bitindex);
                    dcl = dcl << (7 - bitindex);
                    dstRow[byteindex] = (lUInt8)((dstRow[byteindex] & (~mask)) | dcl);
                }
            } else {
                // For other bit depths, just write grayscale value
                lUInt8* dstRow = (lUInt8*)dst->GetScanLine(yy) + dst_x;
                for (int x = 0; x < width; x++) {
                    int xx = x + dst_x;
                    if (xx < clip.left || xx >= clip.right)
                        continue;
                    dstRow[x] = srcRow[x];
                }
            }
        }

        // Swap error buffers
        short* temp = errorCur;
        errorCur = errorNext;
        errorNext = temp;
    }

    delete[] (errorCur - 1);
    delete[] (errorNext - 1);
}
