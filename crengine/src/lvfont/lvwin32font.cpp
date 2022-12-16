/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007,2008,2010,2011,2013 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2018 Frans de Jonge <fransdejonge@gmail.com>            *
 *   Copyright (C) 2019-2021 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2019-2022 Aleksey Chernov <valexlin@gmail.com>          *
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

#include "lvwin32font.h"

#if USE_WIN32_FONTS == 1

#include <lvfnt.h>
#include <lvfntman.h>

#include "../textlang.h"
#include "../lvdrawbuf/lvdrawbuf_utils.h"
#include "lvfontglyphcache.h"
#include "lvgammacorrection.h"

#include <assert.h>

void LVBaseWin32Font::SetAntialiasMode(font_antialiasing_t mode) {
    _aa_mode = mode;
    clearCache();
}

void LVBaseWin32Font::Clear() {
    if (_hfont) {
        DeleteObject(_hfont);
        _hfont = NULL;
        _height = 0;
        _baseline = 0;
    }
}

bool LVBaseWin32Font::Create(const LOGFONTA& lf) {
    if (!IsNull())
        Clear();
    _aa_mode = fontMan->GetAntialiasMode();
    memcpy(&_logfont, &lf, sizeof(LOGFONTA));
    _hfont = CreateFontIndirectA(&lf);
    if (!_hfont)
        return false;
    //memcpy( &_logfont, &lf, sizeof(LOGFONT) );
    // get text metrics
    SelectObject(_drawbuf.GetDC(), _hfont);
    TEXTMETRICW tm;
    GetTextMetricsW(_drawbuf.GetDC(), &tm);
    _logfont.lfHeight = tm.tmHeight;
    _logfont.lfWeight = tm.tmWeight;
    _logfont.lfItalic = tm.tmItalic;
    _logfont.lfCharSet = tm.tmCharSet;
    GetTextFaceA(_drawbuf.GetDC(), sizeof(_logfont.lfFaceName) - 1, _logfont.lfFaceName);
    _height = tm.tmHeight;
    _baseline = _height - tm.tmDescent;
    return true;
}

bool LVBaseWin32Font::Create(int size, int weight, bool italic, css_font_family_t family, lString8 typeface) {
    if (!IsNull())
        Clear();
    _aa_mode = fontMan->GetAntialiasMode();
    //
    LOGFONTA lf = { 0 };

    lf.lfHeight = size;
    lf.lfWeight = weight;
    lf.lfItalic = italic ? 1 : 0;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
#ifdef USE_BITMAP_FONT
    lf.lfQuality = NONANTIALIASED_QUALITY;
#else
    // ANTIALIASED_QUALITY, PROOF_QUALITY
    lf.lfQuality = CLEARTYPE_QUALITY;
#endif
    strncpy(lf.lfFaceName, typeface.c_str(), LF_FACESIZE);
    lf.lfFaceName[LF_FACESIZE - 1] = 0;
    _typeface = typeface;
    _family = family;
    switch (family) {
        case css_ff_serif:
            lf.lfPitchAndFamily = VARIABLE_PITCH | FF_ROMAN;
            break;
        case css_ff_sans_serif:
            lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
            break;
        case css_ff_cursive:
            lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SCRIPT;
            break;
        case css_ff_fantasy:
            lf.lfPitchAndFamily = VARIABLE_PITCH | FF_DECORATIVE;
            break;
        case css_ff_monospace:
            lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
            break;
        default:
            lf.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
            break;
    }
    _hfont = CreateFontIndirectA(&lf);
    if (!_hfont)
        return false;
    //memcpy( &_logfont, &lf, sizeof(LOGFONT) );
    // get text metrics
    SelectObject(_drawbuf.GetDC(), _hfont);
    TEXTMETRICW tm;
    GetTextMetricsW(_drawbuf.GetDC(), &tm);
    memset(&_logfont, 0, sizeof(LOGFONTA));
    _logfont.lfHeight = tm.tmHeight;
    _logfont.lfWeight = tm.tmWeight;
    _logfont.lfItalic = tm.tmItalic;
    _logfont.lfCharSet = tm.tmCharSet;
    GetTextFaceA(_drawbuf.GetDC(), sizeof(_logfont.lfFaceName) - 1, _logfont.lfFaceName);
    _size = size;
    _height = tm.tmHeight;
    _baseline = _height - tm.tmDescent;
    return true;
}

#if USE_WIN32DRAW_FONTS == 1

/// returns char width
int LVWin32DrawFont::getCharWidth(lChar32 ch, lChar32 def_char) {
    if (_hfont == NULL)
        return 0;
    // measure character widths
    GCP_RESULTSW gcpres = { 0 };

    gcpres.lStructSize = sizeof(gcpres);
    lString16 str16 = UnicodeToUtf16(&ch, 1);
    int dx[2];
    gcpres.lpDx = dx;
    gcpres.nMaxFit = 1;
    gcpres.nGlyphs = 1;

    lUInt32 res = GetCharacterPlacementW(
            _drawbuf.GetDC(),
            str16.c_str(),
            str16.length(),
            100,
            &gcpres,
            GCP_MAXEXTENT); //|GCP_USEKERNING

    if (!res) {
        // ERROR
        return 0;
    }

    return dx[0];
}

lUInt32 LVWin32DrawFont::getTextWidth(const lChar32* text, int len, TextLangCfg* lang_cfg) {
    //
    static lUInt16 widths[MAX_LINE_CHARS + 1];
    static lUInt8 flags[MAX_LINE_CHARS + 1];
    if (len > MAX_LINE_CHARS)
        len = MAX_LINE_CHARS;
    if (len <= 0)
        return 0;
    lUInt16 res = measureText(
            text, len,
            widths,
            flags,
            MAX_LINE_WIDTH,
            U' ', // def_char
            lang_cfg);
    if (res > 0 && res < MAX_LINE_CHARS)
        return widths[res - 1];
    return 0;
}

/** \brief measure text
    \param glyph is pointer to glyph_info_t struct to place retrieved info
    \return true if glyph was found 
*/
lUInt16 LVWin32DrawFont::measureText(
        const lChar32* text, int len,
        lUInt16* widths,
        lUInt8* flags,
        int max_width,
        lChar32 def_char,
        TextLangCfg* lang_cfg,
        int letter_spacing,
        bool allow_hyphenation,
        lUInt32 hints,
        lUInt32 fallbackPassMask) {
    if (_hfont == NULL)
        return 0;

    lString16 str16 = UnicodeToUtf16(text, len);
    lChar16* pstr = str16.modify();
    len = str16.length();
    assert(pstr[len] == 0);
    // substitute soft hyphens with zero width spaces
    for (int i = 0; i < len; i++) {
        if (pstr[i] == UNICODE_SOFT_HYPHEN_CODE)
            pstr[i] = UNICODE_ZERO_WIDTH_SPACE;
    }
    assert(pstr[len] == 0);

    // measure character widths
    GCP_RESULTSW gcpres = { 0 };

    gcpres.lStructSize = sizeof(gcpres);
    LVArray<int> dx(len + 1, 0);
    gcpres.lpDx = dx.ptr();
    gcpres.nMaxFit = len;
    gcpres.nGlyphs = len;

    lUInt32 res = GetCharacterPlacementW(
            _drawbuf.GetDC(),
            pstr,
            len,
            max_width,
            &gcpres,
            GCP_MAXEXTENT); //|GCP_USEKERNING
    if (!res) {
        // ERROR
        widths[0] = 0;
        flags[0] = 0;
        return 1;
    }

    if (!_hyphen_width)
        _hyphen_width = getCharWidth(UNICODE_SOFT_HYPHEN_CODE);

    lUInt16 wsum = 0;
    lUInt16 nchars = 0;
    lUInt16 gwidth = 0;
    lUInt8 bflags;
    int isSpace;
    lChar32 ch;
    int hwStart, hwEnd;

    assert(pstr[len] == 0);

    for (; wsum < max_width && nchars < len && nchars < gcpres.nMaxFit; nchars++) {
        bflags = 0;
        ch = text[nchars];
        isSpace = lvfontIsUnicodeSpace(ch);
        if (isSpace || ch == UNICODE_SOFT_HYPHEN_CODE)
            bflags |= LCHAR_ALLOW_WRAP_AFTER;
        if (ch == '-')
            bflags |= LCHAR_DEPRECATED_WRAP_AFTER;
        if (isSpace)
            bflags |= LCHAR_IS_SPACE;
        gwidth = gcpres.lpDx[nchars];
        widths[nchars] = wsum + gwidth;
        if (ch != UNICODE_SOFT_HYPHEN_CODE)
            wsum += gwidth; /* don't include hyphens to width */
        flags[nchars] = bflags;
    }
    //hyphwidth = glyph ? glyph->gi.width : 0;

    //try to add hyphen
    for (hwStart = nchars - 1; hwStart > 0; hwStart--) {
        if (lvfontIsUnicodeSpace(text[hwStart])) {
            hwStart++;
            break;
        }
    }
    for (hwEnd = nchars; hwEnd < len; hwEnd++) {
        lChar32 ch = text[hwEnd];
        if (lvfontIsUnicodeSpace(ch))
            break;
        if (flags[hwEnd - 1] & LCHAR_ALLOW_WRAP_AFTER)
            break;
        if (ch == '.' || ch == ',' || ch == '!' || ch == '?' || ch == '?' || ch == ':' || ch == ';')
            break;
    }
    HyphMan::hyphenate(text + hwStart, hwEnd - hwStart, widths + hwStart, flags + hwStart, _hyphen_width, max_width);
    if (lang_cfg)
        lang_cfg->getHyphMethod()->hyphenate(text + hwStart, hwEnd - hwStart, widths + hwStart, flags + hwStart, _hyphen_width, max_width);
    else // Use global lang hyph method
        HyphMan::hyphenate(text + hwStart, hwEnd - hwStart, widths + hwStart, flags + hwStart, _hyphen_width, max_width);

    return nchars;
}

/// draws text string (returns x advance)
int LVWin32DrawFont::DrawTextString(LVDrawBuf* buf, int x, int y,
                                    const lChar32* text, int len,
                                    lChar32 def_char, lUInt32* palette, bool addHyphen, TextLangCfg* lang_cfg,
                                    lUInt32 flags, int letter_spacing, int width,
                                    int text_decoration_back_gap,
                                    int target_w, int target_h,
                                    lUInt32 fallbackPassMask) {
    if (_hfont == NULL)
        return 0;

    lString16 str16 = UnicodeToUtf16(text, len);
    // substitute soft hyphens with zero width spaces
    if (addHyphen)
        str16 += UNICODE_SOFT_HYPHEN_CODE;
    //str += U"       ";
    lChar16* pstr = str16.modify();
    for (int i = 0; i < len - 1; i++) {
        if (pstr[i] == UNICODE_SOFT_HYPHEN_CODE)
            pstr[i] = UNICODE_ZERO_WIDTH_SPACE;
    }

    lvRect clip;
    buf->GetClipRect(&clip);
    if (y > clip.bottom || y + _height < clip.top)
        return 0;
    if (buf->GetBitsPerPixel() < 16) {
        // draw using backbuffer
        SIZE sz;
        if (!GetTextExtentPoint32W(_drawbuf.GetDC(),
                                   str16.c_str(), str16.length(), &sz))
            return 0;
        LVColorDrawBuf colorbuf(sz.cx, sz.cy);
        colorbuf.Clear(0xFFFFFF);
        HDC dc = colorbuf.GetDC();
        HFONT oldfont = (HFONT)SelectObject(dc, _hfont);
        SetTextColor(dc, 0x000000);
        SetBkMode(dc, TRANSPARENT);
        //ETO_OPAQUE
        if (ExtTextOutW(dc, 0, 0,
                        0, //ETO_OPAQUE
                        NULL,
                        str16.c_str(), str16.length(), NULL)) {
            // COPY colorbuf to buf with converting
            colorbuf.DrawTo(buf, x, y, 0, NULL);
        }
        SelectObject(dc, oldfont);
    } else {
        // draw directly on DC
        //TODO
        HDC dc = ((LVColorDrawBuf*)buf)->GetDC();
        HFONT oldfont = (HFONT)SelectObject(dc, _hfont);
        SetTextColor(dc, RevRGB(buf->GetTextColor()));
        SetBkMode(dc, TRANSPARENT);
        ExtTextOutW(dc, x, y,
                    0, //ETO_OPAQUE
                    NULL,
                    str16.c_str(), str16.length(), NULL);
        SelectObject(dc, oldfont);
    }
    return 0; // advance not implemented
}
#endif // USE_WIN32DRAW_FONTS==1

int LVWin32Font::GetGlyphIndex(HDC hdc, wchar_t code) {
    wchar_t s[2];
    wchar_t g[2];
    s[0] = code;
    s[1] = 0;
    g[0] = 0;
    GCP_RESULTSW gcp;
    gcp.lStructSize = sizeof(GCP_RESULTSW);
    gcp.lpOutString = NULL;
    gcp.lpOrder = NULL;
    gcp.lpDx = NULL;
    gcp.lpCaretPos = NULL;
    gcp.lpClass = NULL;
    gcp.lpGlyphs = g;
    gcp.nGlyphs = 2;
    gcp.nMaxFit = 2;

    DWORD res = GetCharacterPlacementW(
            hdc, s, 1,
            1000,
            &gcp,
            0);
    if (!res)
        return 0;
    return g[0];
}

glyph_t* LVWin32Font::GetGlyphRec(lChar32 ch) {
    glyph_t* p = _cache.get(ch);
    if (p->flgNotExists)
        return NULL;
    if (p->flgValid)
        return p;
    p->flgNotExists = true;
    lUInt16 gi = GetGlyphIndex(_drawbuf.GetDC(), ch);
    if (gi == 0 || gi == 0xFFFF || (gi == _unknown_glyph_index && ch != ' ')) {
        // glyph not found
        p->flgNotExists = true;
        return NULL;
    }
    GLYPHMETRICS metrics;
    p->glyph = NULL;

    MAT2 identity = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
    DWORD res = GetGlyphOutlineW(_drawbuf.GetDC(), ch,
                                 GGO_METRICS,
                                 &metrics,
                                 0,
                                 NULL,
                                 &identity);
    if (res == GDI_ERROR)
        return NULL;
#ifdef USE_BITMAP_FONT
    UINT format = GGO_BITMAP;
#else
    UINT format;
    switch (_aa_mode) {
        case font_aa_none:
            format = GGO_BITMAP;
            break;
        default:
            format = GGO_GRAY8_BITMAP;
            break;
    }
#endif
    DWORD gs = GetGlyphOutlineW(_drawbuf.GetDC(), ch,
                                format,
                                &metrics,
                                0,
                                NULL,
                                &identity);
    if (gs > 0x10000 || gs == GDI_ERROR)
        return NULL;

    p->gi.blackBoxX = metrics.gmBlackBoxX;
    p->gi.blackBoxY = metrics.gmBlackBoxY;
    p->gi.originX = (lInt8)metrics.gmptGlyphOrigin.x;
    p->gi.originY = (lInt8)metrics.gmptGlyphOrigin.y;
    p->gi.width = (lUInt8)metrics.gmCellIncX;

    if (p->gi.blackBoxX > 0 && p->gi.blackBoxY > 0) {
        if (gs > 0) {
            lUInt8* glyph = (lUInt8*)malloc(gs);
            res = GetGlyphOutlineW(_drawbuf.GetDC(), ch,
                                   format,
                                   &metrics,
                                   gs,
                                   glyph,
                                   &identity);
            if (res == GDI_ERROR) {
                free(glyph);
                return NULL;
            }
            p->glyph = glyph;
            if (GGO_GRAY8_BITMAP == format) {
                p->row_size = ((p->gi.blackBoxX + 3) / 4) * 4;
                p->pf = BMP_PIXEL_FORMAT_GRAY;
                // scale values in range '0 - 64' to '0 - 255'
                lUInt8* row = glyph;
                for (int y = 0; y < p->gi.blackBoxY; y++) {
                    for (int x = 0; x < p->gi.blackBoxX; x++) {
                        if (row[x] == 0)
                            row[x] = 0;
                        else if (row[x] >= 64)
                            row[x] = 255;
                        else
                            row[x] = (row[x] << 2) + 2;
                    }
                    row += p->row_size;
                }
            } else {
                p->row_size = ((p->gi.blackBoxX + 31) / 32) * 4;
                p->pf = BMP_PIXEL_FORMAT_MONO;
            }
        } else {
            p->row_size = ((p->gi.blackBoxX + 31) / 32) * 4;
            gs = p->row_size * p->gi.blackBoxY;
            p->glyph = (lUInt8*)malloc(gs);
            p->pf = BMP_PIXEL_FORMAT_MONO;
            memset(p->glyph, 0, gs);
        }
    } else {
        p->row_size = 0;
        p->pf = BMP_PIXEL_FORMAT_MONO;
    }
    // found!
    p->flgValid = true;
    p->flgNotExists = false;
    return p;
}

void LVWin32Font::setGammaIndex(int index) {
    _gammaIndex = index;
    clearCache();
}

/** \brief get glyph info
    \param glyph is pointer to glyph_info_t struct to place retrieved info
    \return true if glyh was found 
*/
bool LVWin32Font::getGlyphInfo(lUInt32 code, glyph_info_t* glyph, lChar32 def_char, lUInt32 fallbackPassMask) {
    if (_hfont == NULL)
        return false;
    glyph_t* p = GetGlyphRec(code);
    if (!p)
        return false;
    *glyph = p->gi;
    return true;
}

lUInt32 LVWin32Font::getTextWidth(const lChar32* text, int len, TextLangCfg* lang_cfg) {
    //
    static lUInt16 widths[MAX_LINE_CHARS + 1];
    static lUInt8 flags[MAX_LINE_CHARS + 1];
    if (len > MAX_LINE_CHARS)
        len = MAX_LINE_CHARS;
    if (len <= 0)
        return 0;
    lUInt16 res = measureText(
            text, len,
            widths,
            flags,
            MAX_LINE_WIDTH,
            U' ', // def_char
            lang_cfg);
    if (res > 0 && res < MAX_LINE_CHARS)
        return widths[res - 1];
    return 0;
}

/** \brief measure text
    \param glyph is pointer to glyph_info_t struct to place retrieved info
    \return true if glyph was found 
*/
lUInt16 LVWin32Font::measureText(
        const lChar32* text, int len,
        lUInt16* widths,
        lUInt8* flags,
        int max_width,
        lChar32 def_char,
        TextLangCfg* lang_cfg,
        int letter_spacing,
        bool allow_hyphenation,
        lUInt32 hints,
        lUInt32 fallbackPassMask) {
    if (_hfont == NULL)
        return 0;

    lUInt16 wsum = 0;
    lUInt16 nchars = 0;
    glyph_t* glyph; //GetGlyphRec( lChar32 ch )
    lUInt16 gwidth = 0;
    lUInt8 bflags;
    int isSpace;
    lChar32 ch;
    int hwStart, hwEnd;

    if (!_hyphen_width)
        _hyphen_width = getCharWidth(UNICODE_SOFT_HYPHEN_CODE);

    for (; wsum < max_width && nchars < len; nchars++) {
        bflags = 0;
        ch = text[nchars];
        isSpace = lvfontIsUnicodeSpace(ch);
        if (isSpace || ch == UNICODE_SOFT_HYPHEN_CODE)
            bflags |= LCHAR_ALLOW_WRAP_AFTER;
        if (ch == '-')
            bflags |= LCHAR_DEPRECATED_WRAP_AFTER;
        if (isSpace)
            bflags |= LCHAR_IS_SPACE;
        glyph = GetGlyphRec(ch);
        if (!glyph && def_char)
            glyph = GetGlyphRec(def_char);
        gwidth = glyph ? glyph->gi.width : 0;
        widths[nchars] = wsum + gwidth;
        if (ch != UNICODE_SOFT_HYPHEN_CODE)
            wsum += gwidth; /* don't include hyphens to width */
        flags[nchars] = bflags;
    }

    //try to add hyphen
    for (hwStart = nchars - 1; hwStart > 0; hwStart--) {
        if (lvfontIsUnicodeSpace(text[hwStart])) {
            hwStart++;
            break;
        }
    }
    for (hwEnd = nchars; hwEnd < len; hwEnd++) {
        lChar32 ch = text[hwEnd];
        if (lvfontIsUnicodeSpace(ch))
            break;
        if (flags[hwEnd - 1] & LCHAR_ALLOW_WRAP_AFTER)
            break;
        if (ch == '.' || ch == ',' || ch == '!' || ch == '?' || ch == '?')
            break;
    }
    if (lang_cfg)
        lang_cfg->getHyphMethod()->hyphenate(text + hwStart, hwEnd - hwStart, widths + hwStart, flags + hwStart, _hyphen_width, max_width);
    else // Use global lang hyph method
        HyphMan::hyphenate(text + hwStart, hwEnd - hwStart, widths + hwStart, flags + hwStart, _hyphen_width, max_width);

    return nchars;
}

static LVFontGlyphCacheItem* newItem(LVFontLocalGlyphCache* local_cache, lChar32 ch, glyph_t* g, int gammaIndex) {
    FONT_LOCAL_GLYPH_CACHE_GUARD
    unsigned int w = g->gi.blackBoxX;
    unsigned int h = g->gi.blackBoxY;
    unsigned int bmp_sz = g->row_size * g->gi.blackBoxY;
    LVFontGlyphCacheItem* item = LVFontGlyphCacheItem::newItem(local_cache, ch, w, h, g->row_size, bmp_sz);
    if (!item)
        return 0;
    item->bmp_fmt = g->pf;
    if (g->glyph && w > 0 && h > 0 && bmp_sz > 0) {
        memcpy(item->bmp, g->glyph, bmp_sz);
        // apply gamma
        switch (item->bmp_fmt) {
            case BMP_PIXEL_FORMAT_GRAY2:
            case BMP_PIXEL_FORMAT_GRAY4:
                // TODO: implement this
                break;
            case BMP_PIXEL_FORMAT_GRAY:
                // correct gamma
                if (gammaIndex != LVGammaCorrection::NoCorrectionIndex)
                    LVGammaCorrection::gammaCorrection(item->bmp, bmp_sz, gammaIndex);
                break;
            case BMP_PIXEL_FORMAT_RGB:
            case BMP_PIXEL_FORMAT_BGR:
            case BMP_PIXEL_FORMAT_RGB_V:
            case BMP_PIXEL_FORMAT_BGR_V:
            case BMP_PIXEL_FORMAT_BGRA:
                // not applicable here
                break;
            case BMP_PIXEL_FORMAT_MONO:
                // just ignore gamma on mono color image
                break;
        }
    }
    item->origin_x = (lInt16)g->gi.originX;
    item->origin_y = (lInt16)g->gi.originY;
    item->advance = (lUInt16)g->gi.width;
    return item;
}

LVFontGlyphCacheItem* LVWin32Font::getGlyph(lUInt32 ch, lChar32 def_char, lUInt32 fallbackPassMask) {
    LVFontGlyphCacheItem* item = _glyph_cache.get(ch);
    if (!item) {
        if (_hfont != NULL) {
            glyph_t* p = GetGlyphRec(ch);
            if (p) {
                item = newItem(&_glyph_cache, ch, p, _gammaIndex);
                if (item)
                    _glyph_cache.put(item);
            }
        }
    }
    return item;
}

void LVWin32Font::Clear() {
    LVBaseWin32Font::Clear();
    _cache.clear();
    _glyph_cache.clear();
}

bool LVWin32Font::Create(const LOGFONTA& lf) {
    if (!LVBaseWin32Font::Create(lf))
        return false;
    _unknown_glyph_index = GetGlyphIndex(_drawbuf.GetDC(), 1);
    return true;
}

bool LVWin32Font::Create(int size, int weight, bool italic, css_font_family_t family, lString8 typeface) {
    if (!LVBaseWin32Font::Create(size, weight, italic, family, typeface))
        return false;
    _unknown_glyph_index = GetGlyphIndex(_drawbuf.GetDC(), 1);
    return true;
}

void LVWin32Font::clearCache() {
    _glyph_cache.clear();
    _cache.clear();
}

#endif // USE_WIN32_FONTS == 1
