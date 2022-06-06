/**
 *  crengine-ng
 *  Copyright (c) 2022 Aleksey Chernov
 *
 *  \file crengine-ng-environment.cpp
 *  \brief crengine-ng unit test global test environment implementation.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <crlog.h>
#include <crhyphman.h>
#include <lvstring32collection.h>
#include <lvfntman.h>
#include <lvstreamutils.h>
#include <lvcontaineriteminfo.h>

#include "gtest/gtest.h"

#ifndef HYPH_DIR
#define HYPH_DIR "./"
#endif

#ifndef FONTS_DIR
#define FONTS_DIR "./"
#endif

bool InitFonts(lString32Collection& fontDirs);
#if (USE_FREETYPE == 1)
bool getDirectoryFonts(lString32Collection& pathList, lString32Collection& ext, lString32Collection& fonts, bool absPath);
#endif

class TinyDomEnvironment: public ::testing::Environment
{
    bool m_initOK;
public:
    ~TinyDomEnvironment() override { }

    virtual void SetUp() override {
        LVDeleteFile(U"crengine-ng.log");
        CRLog::setFileLogger("crengine-ng.log", true);
        CRLog::setLogLevel(CRLog::LL_TRACE);
        // init engine: font manager, etc
        lString32Collection fontDirs;
        // Use only one platform independent font set
        lString32 fontsdir = cs32(FONTS_DIR);
        fontDirs.add(fontsdir);
        bool fonts_ok = InitFonts(fontDirs);
        if (!fonts_ok)
            fprintf(stderr, "Cannot init crengine-ng (fonts)\n");
        // init hyphman
        HyphMan::initDictionaries(Utf8ToUnicode(HYPH_DIR));
        bool hyph_ok = HyphMan::activateDictionary(cs32("Russian_EnUS.pattern"));
        if (!hyph_ok)
            fprintf(stderr, "Cannot init crengine-ng (hyphenation dicts)\n");
        m_initOK = fonts_ok && hyph_ok;
    }

    virtual void TearDown() override {
        HyphMan::uninit();
        ShutdownFontManager();
        CRLog::setLogger(NULL);
    }
};

bool InitFonts(lString32Collection& fontDirs) {
    CRLog::trace("InitCREngine()");
    // Don't enumerate installed system fonts.
    // Use only specified local.
    InitFontManager(lString8::empty_str, false);
    // Load font definitions into font manager
#if USE_FREETYPE == 1
    lString32Collection fontExt;
    fontExt.add(cs32(".ttf"));
    fontExt.add(cs32(".ttc"));
    fontExt.add(cs32(".otf"));
    fontExt.add(cs32(".pfa"));
    fontExt.add(cs32(".pfb"));

    lString32Collection fonts;
    getDirectoryFonts(fontDirs, fontExt, fonts, true);

    // load fonts from file
    CRLog::debug("%d font files found", fonts.length());
    for (int fi = 0; fi < fonts.length(); fi++) {
        lString8 fn = UnicodeToLocal(fonts[fi]);
        CRLog::trace("loading font: %s", fn.c_str());
        if (!fontMan->RegisterFont(fn)) {
            CRLog::trace("    failed\n");
        }
    }
#endif // USE_FREETYPE==1
    if (!fontMan->GetFontCount()) {
        //error
#if (USE_FREETYPE == 1)
        fprintf(stderr, "Fatal Error: Cannot open font file(s) .ttf \nCannot work without font\n");
#else
        fprintf(stderr, "Fatal Error: Cannot open font file(s) font#.lbf \nCannot work without font\nUse FontConv utility to generate .lbf fonts from TTF\n");
#endif
        return false;
    }
    printf("%d fonts loaded.\n", fontMan->GetFontCount());
    return true;
}

#if USE_FREETYPE == 1
bool getDirectoryFonts(lString32Collection& pathList, lString32Collection& ext, lString32Collection& fonts, bool absPath) {
    int foundCount = 0;
    lString32 path;
    for (int di = 0; di < pathList.length(); di++) {
        path = pathList[di];
        LVContainerRef dir = LVOpenDirectory(path.c_str());
        if (!dir.isNull()) {
            CRLog::trace("Checking directory %s", UnicodeToUtf8(path).c_str());
            for (int i = 0; i < dir->GetObjectCount(); i++) {
                const LVContainerItemInfo* item = dir->GetObjectInfo(i);
                lString32 fileName = item->GetName();
                //lString8 fn = UnicodeToLocal(fileName);
                //printf(" test(%s) ", fn.c_str() );
                if (!item->IsContainer()) {
                    bool found = false;
                    lString32 lc = fileName;
                    lc.lowercase();
                    for (int j = 0; j < ext.length(); j++) {
                        if (lc.endsWith(ext[j])) {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        continue;
                    lString32 fn;
                    if (absPath) {
                        fn = path;
                        if (!fn.empty() && fn[fn.length() - 1] != PATH_SEPARATOR_CHAR)
                            fn << PATH_SEPARATOR_CHAR;
                    }
                    fn << fileName;
                    foundCount++;
                    fonts.add(fn);
                }
            }
        }
    }
    return foundCount > 0;
}
#endif

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    AddGlobalTestEnvironment(new TinyDomEnvironment);
    return RUN_ALL_TESTS();
}
