/***************************************************************************
 *   crengine-ng, unit testing                                             *
 *   Copyright (C) 2023 Aleksey Chernov <valexlin@gmail.com>               *
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
 * \file fontman_funcs_tests.cpp
 * \brief Tests various functions of the LVFontManager classes.
 */

#include <crlog.h>
#include <lvfntman.h>

#if (USE_FREETYPE == 1) && (USE_LOCALE_DATA == 1)

#include "gtest/gtest.h"

// units tests

TEST(FontManFuncsTests, TestGetFaceListFiltered) {
    CRLog::info("================================");
    CRLog::info("Starting TestGetFaceListFiltered");

    lString32Collection allFaces;
    lString32Collection monoFaces;
    lString32Collection monoFacesForEN;
    lString32Collection monoFacesForZH;
    lString32Collection monoFacesForInvalidLang;

    // get all registered fonts faces
    fontMan->getFaceList(allFaces);
    // get all registered monospace fonts faces
    fontMan->getFaceListFiltered(monoFaces, css_ff_monospace, lString8::empty_str);
    // get all registered monospace fonts faces for "en" language tag
    fontMan->getFaceListFiltered(monoFacesForEN, css_ff_monospace, cs8("en"));
    // get all registered monospace fonts faces for "zh" language tag
    fontMan->getFaceListFiltered(monoFacesForZH, css_ff_monospace, cs8("zh"));
    // get all registered monospace fonts faces for invalid language tag
    fontMan->getFaceListFiltered(monoFacesForInvalidLang, css_ff_monospace, cs8("inv-INV"));

    EXPECT_GE(allFaces.length(), 0);
    EXPECT_GE(allFaces.length(), monoFaces.length());
    EXPECT_EQ(monoFaces.length(), 1);
    EXPECT_STREQ(LCSTR(monoFaces[0]), "FreeMono");
    EXPECT_EQ(monoFacesForEN.length(), 1);
    EXPECT_STREQ(LCSTR(monoFacesForEN[0]), "FreeMono");
    EXPECT_EQ(monoFacesForZH.length(), 0);
    EXPECT_EQ(monoFacesForInvalidLang.length(), 0);

    CRLog::info("Finished TestGetFaceListFiltered");
    CRLog::info("================================");
}

TEST(FontManFuncsTests, TestCheckFontLangCompat) {
    CRLog::info("================================");
    CRLog::info("Starting TestCheckFontLangCompat");

    EXPECT_EQ(fontMan->checkFontLangCompat(cs8("FreeSans"), cs8("en")), font_lang_compat_full);
    EXPECT_EQ(fontMan->checkFontLangCompat(cs8("FreeSans"), cs8("de")), font_lang_compat_full);
    EXPECT_EQ(fontMan->checkFontLangCompat(cs8("FreeSans"), cs8("ar")), font_lang_compat_none);
    EXPECT_EQ(fontMan->checkFontLangCompat(cs8("FreeSans"), cs8("zh")), font_lang_compat_none);
    EXPECT_EQ(fontMan->checkFontLangCompat(cs8("FreeSans"), cs8("invalid")), font_lang_compat_invalid_tag);
    EXPECT_EQ(fontMan->checkFontLangCompat(cs8("NotExistingFont"), cs8("en")), font_lang_compat_none);
    EXPECT_EQ(fontMan->checkFontLangCompat(cs8("NotExistingFont"), cs8("invalid")), font_lang_compat_invalid_tag);

    CRLog::info("Finished TestCheckFontLangCompat");
    CRLog::info("================================");
}

#endif // (USE_FREETYPE == 1) && (USE_LOCALE_DATA == 1)
