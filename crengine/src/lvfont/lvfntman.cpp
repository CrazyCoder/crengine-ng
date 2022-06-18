/** @file lvfntman.cpp
    @brief font manager implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include <lvfntman.h>
#include <lvstyles.h>
#include <crlog.h>

#include "lvfreetypefontman.h"
#include "lvwin32fontman.h"
#include "lvbitmapfontman.h"
#include "lvgammacorrection.h"

LVFontManager* fontMan = NULL;

/// returns first found face from passed list, or return face for font found by family only
LVFontManager::LVFontManager()
        : _allowKerning(false)
        , _antialiasMode(font_aa_all)
        , _shapingMode(SHAPING_MODE_FREETYPE)
        , _hintingMode(HINTING_MODE_AUTOHINT)
        , _gammaIndex(LVGammaCorrection::NoCorrectionIndex) {
}

LVFontManager::~LVFontManager() {
}

float LVFontManager::GetGamma() {
    return LVGammaCorrection::getIndex(_gammaIndex);
}

void LVFontManager::SetGamma(double gamma) {
    int index = LVGammaCorrection::getIndex(gamma);
    if (_gammaIndex != index) {
        CRLog::trace("FontManager gamma index is changed from %d to %d", _gammaIndex, index);
        _gammaIndex = index;
        gc();
        clearGlyphCache();
    }
}

lString8 LVFontManager::findFontFace(lString8 commaSeparatedFaceList, css_font_family_t fallbackByFamily) {
    // faces we want
    lString8Collection list;
    splitPropertyValueList(commaSeparatedFaceList.c_str(), list);
    // faces we have
    lString32Collection faces;
    getFaceList(faces);
    // find first matched
    for (int i = 0; i < list.length(); i++) {
        lString8 wantFace = list[i];
        for (int j = 0; j < faces.length(); j++) {
            lString32 haveFace = faces[j];
            if (wantFace == haveFace)
                return wantFace;
        }
    }
    // not matched - get by family name
    LVFontRef fnt = GetFont(10, 400, false, fallbackByFamily, lString8("Arial"));
    if (fnt.isNull())
        return lString8::empty_str; // not found
    // get face from found font
    return fnt->getTypeFace();
}

bool InitFontManager(lString8 path, bool initSystemFonts) {
    if (fontMan) {
        return true;
        //delete fontMan;
    }
#if (USE_WIN32_FONTS == 1)
    fontMan = new LVWin32FontManager;
#elif (USE_FREETYPE == 1)
    fontMan = new LVFreeTypeFontManager;
#else
    fontMan = new LVBitmapFontManager;
#endif
    return fontMan->Init(path, initSystemFonts);
}

bool ShutdownFontManager() {
    if (fontMan) {
        delete fontMan;
        fontMan = NULL;
        return true;
    }
    return false;
}
