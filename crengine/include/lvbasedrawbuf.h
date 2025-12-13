/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007,2011,2012 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2015 Yifei(Frank) ZHU <fredyifei@gmail.com>             *
 *   Copyright (C) 2019 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2019,2021 NiLuJe <ninuje@gmail.com>                     *
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

/**
 * \file lvbasedrawbuf.h
 * \brief Drawing buffer, common bitmap buffer
 */

#ifndef __LVBASEDRAWBUF_H_INCLUDED__
#define __LVBASEDRAWBUF_H_INCLUDED__

#include <lvdrawbuf.h>

/// Options for Floyd-Steinberg dithering algorithm
struct DitheringOptions {
    /// Threshold for black/white decision (0.0-1.0, default 0.5)
    /// Higher values produce darker output, lower values produce brighter output
    /// For 1-bit: pixels >= threshold*255 become white
    /// For 2-bit: affects quantization boundaries proportionally
    float threshold;

    /// Error diffusion strength (0.0-1.0, default 1.0)
    /// 1.0 = full Floyd-Steinberg error propagation
    /// Lower values reduce noise but may lose tonal accuracy in gradients
    /// 0.0 = no error diffusion (simple threshold)
    float errorDiffusion;

    /// Input gamma correction (0.5-2.5, default 1.0)
    /// Applied to input grayscale values before dithering
    /// >1.0 darkens midtones, <1.0 brightens midtones
    float gamma;

    /// Serpentine scanning (default true)
    /// Alternates scan direction each row to reduce directional artifacts
    bool serpentine;

    /// Default constructor with sensible defaults
    DitheringOptions()
        : threshold(0.5f)
        , errorDiffusion(1.0f)
        , gamma(1.0f)
        , serpentine(true) {}

    /// Constructor with all parameters
    DitheringOptions(float thresh, float errDiff, float gam, bool serp)
        : threshold(thresh)
        , errorDiffusion(errDiff)
        , gamma(gam)
        , serpentine(serp) {}
};

/**
 * @brief Image dithering mode for grayscale conversion
 *
 * Controls how images are dithered when drawn to low bit-depth buffers.
 * Text rendering is unaffected by this setting.
 */
enum ImageDitherMode {
    IMAGE_DITHER_NONE = 0,      ///< No dithering, simple quantization
    IMAGE_DITHER_ORDERED = 1,   ///< Ordered (Bayer 8x8) dithering - default
    IMAGE_DITHER_FS_2BIT = 2,   ///< Floyd-Steinberg dithering to 2-bit (4 levels)
    IMAGE_DITHER_FS_1BIT = 3    ///< Floyd-Steinberg dithering to 1-bit (2 levels)
};

/// Get default dithering options for 1-bit mode
/// Tuned for e-ink displays: slightly darker
DitheringOptions getDefault1BitDitheringOptions();

/// Get default dithering options for 2-bit mode
/// More levels need less aggressive noise reduction
DitheringOptions getDefault2BitDitheringOptions();

/// LVDrawBufferBase
class LVBaseDrawBuf: public LVDrawBuf
{
protected:
    int _dx;
    int _dy;
    int _rowsize;
    lvRect _clip;
    unsigned char* _data;
    lUInt32 _backgroundColor;
    lUInt32 _textColor;
    bool _hidePartialGlyphs;
    bool _invertImages;
    ImageDitherMode _imageDitherMode;
    const DitheringOptions* _ditheringOptions;
    bool _smoothImages;
    int _drawnImagesCount;
    int _drawnImagesSurface;
public:
    /// set to true for drawing in Paged mode, false for Scroll mode
    virtual void setHidePartialGlyphs(bool hide) {
        _hidePartialGlyphs = hide;
    }
    /// set to true to invert images only (so they get inverted back to normal by nightmode)
    virtual void setInvertImages(bool invert) {
        _invertImages = invert;
    }
    /// set to true to enforce dithering (only relevant for 8bpp Gray drawBuf)
    /// @deprecated Use setImageDitherMode() instead
    virtual void setDitherImages(bool dither) {
        _imageDitherMode = dither ? IMAGE_DITHER_ORDERED : IMAGE_DITHER_NONE;
    }
    /// set image dithering mode for grayscale conversion
    virtual void setImageDitherMode(ImageDitherMode mode) {
        _imageDitherMode = mode;
    }
    /// get current image dithering mode
    virtual ImageDitherMode getImageDitherMode() const {
        return _imageDitherMode;
    }
    /// set custom dithering options for Floyd-Steinberg modes
    /// @param options Pointer to options (caller owns memory, must remain valid during drawing)
    ///                Pass nullptr to use defaults
    virtual void setDitheringOptions(const DitheringOptions* options) {
        _ditheringOptions = options;
    }
    /// get current dithering options (may be nullptr for defaults)
    virtual const DitheringOptions* getDitheringOptions() const {
        return _ditheringOptions;
    }
    /// set to true to switch to a more costly smooth scaler instead of nearest neighbor
    virtual void setSmoothScalingImages(bool smooth) {
        _smoothImages = smooth;
    }
    /// returns current background color
    virtual lUInt32 GetBackgroundColor() const {
        return _backgroundColor;
    }
    /// sets current background color
    virtual void SetBackgroundColor(lUInt32 cl) {
        _backgroundColor = cl;
    }
    /// returns current text color
    virtual lUInt32 GetTextColor() const {
        return _textColor;
    }
    /// sets current text color
    virtual void SetTextColor(lUInt32 cl) {
        _textColor = cl;
    }
    /// gets clip rect
    virtual void GetClipRect(lvRect* clipRect) const {
        *clipRect = _clip;
    }
    /// sets clip rect
    virtual void SetClipRect(const lvRect* clipRect);
    /// get average pixel value for area (coordinates are fixed floating points *16)
    virtual lUInt32 GetAvgColor(lvRect& rc16) const;
    /// get linearly interpolated pixel value (coordinates are fixed floating points *16)
    virtual lUInt32 GetInterpolatedColor(int x16, int y16) const;
    /// get buffer width, pixels
    virtual int GetWidth() const {
        return _dx;
    }
    /// get buffer height, pixels
    virtual int GetHeight() const {
        return _dy;
    }
    /// get row size (bytes)
    virtual int GetRowSize() const {
        return _rowsize;
    }
    virtual void DrawLine(int x0, int y0, int x1, int y1, lUInt32 color0, int length1, int length2, int direction) = 0;
    /// Get nb of images drawn on buffer
    int getDrawnImagesCount() const {
        return _drawnImagesCount;
    }
    /// Get surface of images drawn on buffer
    int getDrawnImagesSurface() const {
        return _drawnImagesSurface;
    }

    LVBaseDrawBuf()
            : _dx(0)
            , _dy(0)
            , _rowsize(0)
            , _data(NULL)
            , _hidePartialGlyphs(true)
            , _invertImages(false)
            , _imageDitherMode(IMAGE_DITHER_ORDERED) // for 1-bit and 2-bit e-ink displays
            , _ditheringOptions(nullptr)
            , _smoothImages(false)
            , _drawnImagesCount(0)
            , _drawnImagesSurface(0) { }
    virtual ~LVBaseDrawBuf() { }
};

#endif // __LVBASEDRAWBUF_H_INCLUDED__
