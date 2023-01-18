/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007,2008,2011,2012,2014 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2020 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2020-2023 Aleksey Chernov <valexlin@gmail.com>          *
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

#include "lvwin32fontman.h"

#if USE_WIN32_FONTS == 1

#include "lvwin32font.h"
#include "lvfontboldtransform.h"
#include "lvgammacorrection.h"

#include <crlog.h>

//#define DEBUG_FONT_MAN 1

// prototype
int CALLBACK LVWin32FontEnumFontFamExProc(
        const LOGFONTA* lpelfe,    // logical-font data
        const TEXTMETRICA* lpntme, // physical-font data
        //ENUMLOGFONTEX *lpelfe,    // logical-font data
        //NEWTEXTMETRICEX *lpntme,  // physical-font data
        DWORD FontType, // type of font
        LPARAM lParam   // application-defined data
);

lString8 faceName(const char* name) {
    lString8 face(name);
    static const char* patterns[] = {
        " Thin",       // weight 100
        " ExtraLight", // weight 200
        " Light",      // weight 300
        " SemiLight",  // weight 350
        " Semilight",  // weight 350
        "-Regular",    // weight 400
        " Regular",    // weight 400
        " Medium",     // weight 500
        " DemiBold",   // weight 600
        " SemiBold",   // weight 600
        " Semibold",   // weight 600
        " Bold",       // weight 700
        " extrabold",  // weight 800
        " Black",      // weight 900
        " ExtraBlack", // weight 950
        " UltraBlack", // weight 950
        0
    };
    for (int i = 0; patterns[i]; i++) {
        int pos = face.rpos(patterns[i]);
        if (pos > 0) {
            lString8 tail = face.substr(pos + strlen(patterns[i]));
            face = face.substr(0, pos) + tail;
        }
    }
    return face;
}

LVWin32FontManager::~LVWin32FontManager() {
    _globalCache.clear();
    _cache.clear();
    //if (_log)
    //    fclose(_log);
}

LVWin32FontManager::LVWin32FontManager()
        : _globalCache(GLYPH_CACHE_SIZE) {
    //_log = fopen( "fonts.log", "wt" );
}

LVFontRef LVWin32FontManager::GetFont(int size, int weight, bool bitalic, css_font_family_t family, lString8 typeface,
                                      int features, int documentId, bool useBias) {
    int italic = bitalic ? 1 : 0;
    if (size < 8)
        size = 8;
    if (size > 255)
        size = 255;

    LVFontDef def(
            lString8::empty_str,
            size,
            weight,
            italic,
            0,
            family,
            typeface,
            -1,
            documentId);

    //fprintf( _log, "GetFont: %s %d %s %s\n",
    //    typeface.c_str(),
    //    size,
    //    weight>400?"bold":"",
    //    italic?"italic":"" );
    LVFontCacheItem* item = _cache.find(&def, useBias);

    if (NULL == item) {
        CRLog::error("_cache.find() return NULL: size=%d, weight=%d, italic=%d, family=%d, typeface=%s", size, weight, italic, family, typeface.c_str());
        CRLog::error("possible font cache cleared!");
        return LVFontRef(NULL);
    }

    LVFontDef newDef(*item->getDef());

    if (!item->getFont().isNull()) {
        int deltaWeight = weight - item->getDef()->getWeight();
        if (deltaWeight >= 200) {
            // This instantiated cached font has a too low weight
            // embolden using LVFontBoldTransform
            CRLog::debug("font: apply Embolding to increase weight from %d to %d",
                         newDef.getWeight(), newDef.getWeight() + 200);
            newDef.setWeight(newDef.getWeight() + 200);
            LVFontRef ref = LVFontRef(new LVFontBoldTransform(item->getFont(), &_globalCache));
            _cache.update(&newDef, ref);
            return ref;
        } else {
            //fprintf(_log, "    : fount existing\n");
            return item->getFont();
        }
    }

#if COLOR_BACKBUFFER == 1 && USE_WIN32DRAW_FONTS == 1
    LVWin32DrawFont* font = new LVWin32DrawFont;
#else
    LVWin32Font* font = new LVWin32Font(&_globalCache);
#endif

    if (font->Create(size, weight, italic ? true : false, item->getDef()->getFamily(), item->getDef()->getName())) {
        //fprintf(_log, "    : loading from file %s : %s %d\n", item->getDef()->getName().c_str(),
        //    item->getDef()->getTypeFace().c_str(), item->getDef()->getSize() );
        LVFontRef ref(font);
        font->setFeatures(0);
        font->setKerning(GetKerning());
        font->setShapingMode(GetShapingMode());
        font->setGammaIndex(_gammaIndex);
        //font->setFaceName(item->getDef()->getTypeFace());
        newDef.setSize(size);
        _cache.addInstance(&newDef, ref);
        return ref;
    }
    delete font;
    return LVFontRef(NULL);
}

void LVWin32FontManager::GetAvailableFontWeights(LVArray<int>& weights, lString8 typeface) {
    _cache.getAvailableFontWeights(weights, typeface);
}

bool LVWin32FontManager::RegisterFont(const ENUMLOGFONTEXA* lpelfe) {
    lString8 face = faceName((const char*)lpelfe->elfFullName);
    css_font_family_t ff;
    switch (lpelfe->elfLogFont.lfPitchAndFamily & 0x70) {
        case FF_ROMAN:
            ff = css_ff_serif;
            break;
        case FF_SWISS:
            ff = css_ff_sans_serif;
            break;
        case FF_SCRIPT:
            ff = css_ff_cursive;
            break;
        case FF_DECORATIVE:
            ff = css_ff_fantasy;
            break;
        case FF_MODERN:
            ff = css_ff_monospace;
            break;
        default:
            ff = css_ff_sans_serif;
            break;
    }
#if DEBUG_FONT_MAN == 1
    CRLog::trace("register font: %s; w:%d", face.c_str(), lpelfe->elfLogFont.lfWeight);
#endif
    LVFontDef def(
            lString8(lpelfe->elfLogFont.lfFaceName),
            -1,                               // lf->lfHeight > 0 ? lf->lfHeight : -lf->lfHeight,
            lpelfe->elfLogFont.lfWeight,      // -1,
            lpelfe->elfLogFont.lfItalic != 0, //-1,
            -1,
            ff,
            face);
    _cache.update(&def, LVFontRef(NULL));
    return true;
}

bool LVWin32FontManager::Init(lString8 path, bool initSystemFonts) {
    LVColorDrawBuf drawbuf(1, 1);
    LOGFONTA lf;
    memset(&lf, 0, sizeof(lf));
    lf.lfCharSet = ANSI_CHARSET;
    int res = true;
    if (initSystemFonts) {
        res = EnumFontFamiliesExA(
                drawbuf.GetDC(),              // handle to DC
                &lf,                          // font information
                LVWin32FontEnumFontFamExProc, // callback function (FONTENUMPROC)
                (LPARAM)this,                 // additional data
                0                             // not used; must be 0
        );
    }
    return res != 0;
}

void LVWin32FontManager::clearGlyphCache() {
    FONT_MAN_GUARD
    _globalCache.clear();
}

void LVWin32FontManager::SetAntialiasMode(font_antialiasing_t mode) {
    LVFontManager::SetAntialiasMode(mode);
    FONT_MAN_GUARD
    LVPtrVector<LVFontCacheItem>* fonts = _cache.getInstances();
    for (int i = 0; i < fonts->length(); i++) {
        LVFontRef font = fonts->get(i)->getFont();
        font->SetAntialiasMode(_antialiasMode);
    }
}

float LVWin32FontManager::GetGamma() {
    return LVFontManager::GetGamma();
}

void LVWin32FontManager::SetGamma(double gamma) {
    FONT_MAN_GUARD
    int index = LVGammaCorrection::getIndex(gamma);
    if (_gammaIndex != index) {
        CRLog::debug("Gamma correction index is changed from %d to %d", _gammaIndex, index);
        _gammaIndex = index;
        gc();
        clearGlyphCache();
        LVPtrVector<LVFontCacheItem>* fonts = _cache.getInstances();
        for (int i = 0; i < fonts->length(); i++) {
            LVFontRef font = fonts->get(i)->getFont();
            font->setGammaIndex(_gammaIndex);
        }
    }
}

void LVWin32FontManager::getFaceListFiltered(lString32Collection& list, css_font_family_t family, const lString8& langTag = lString8::empty_str) {
    FONT_MAN_GUARD
    _cache.getFaceListForFamily(list, family);
}

void LVWin32FontManager::getFontFileNameList(lString32Collection& list) {
    FONT_MAN_GUARD
    _cache.getFontFileNameList(list);
}

// definition
int CALLBACK LVWin32FontEnumFontFamExProc(
        const LOGFONTA* lf,        // logical-font data
        const TEXTMETRICA* lpntme, // physical-font data
        DWORD FontType,            // type of font
        LPARAM lParam              // application-defined data
) {
    //
    ENUMLOGFONTEXA* lpelfe = (ENUMLOGFONTEXA*)lf;
#if DEBUG_FONT_MAN == 1
    CRLog::trace("font enum: %s; w:%d; t:%d", lpelfe->elfFullName, lf->lfWeight, FontType);
#endif
    if (FontType == TRUETYPE_FONTTYPE || FontType == DEVICE_FONTTYPE) {
        LVWin32FontManager* fontman = (LVWin32FontManager*)lParam;
        LVWin32Font fnt(&fontman->_globalCache);
        //if (strcmp(lf->lfFaceName, "Courier New"))
        //    return 1;
        if (fnt.Create(*lf)) {
            //
            //static lChar32 chars[] = { 0, 0xBF, 0xE9, 0x106, 0x410, 0x44F, 0 };
            static lChar32 chars[] = { 0x20, 0x3F, 0x2C, 0x2E, 0x41, 0x61, 0 };
            for (int i = 0; chars[i]; i++) {
                LVFont::glyph_info_t glyph;
                if (!fnt.getGlyphInfo(chars[i], &glyph, U' ')) { //def_char
#if DEBUG_FONT_MAN == 1
                    CRLog::trace("bad font '%s': missing main chars...", lf->lfFaceName);
#endif
                    return 1;
                }
            }
            fontman->RegisterFont(lpelfe); //&lpelfe->elfLogFont
        }
        // For some reason, bold fonts are not enumerated...
    }
    return 1;
}

#endif // USE_WIN32_FONTS == 1
