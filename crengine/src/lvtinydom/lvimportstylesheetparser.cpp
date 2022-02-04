/**
 * CoolReader Engine
 *
 * (c) Vadim Lopatin, 2000-2011
 * (c) Other CoolReader authors (See AUTHORS file)
 *
 * This source code is distributed under the terms of
 * GNU General Public License version 2
 * See LICENSE file for details
 */

#include "lvimportstylesheetparser.h"

#include <ldomdocument.h>
#include <lvstreamutils.h>
#include <lvstsheet.h>

#include "../lvxml/lvxmlutils.h"

bool LVImportStylesheetParser::Parse(lString32 cssFile) {
    bool ret = false;
    if (cssFile.empty())
        return ret;

    lString32 codeBase = cssFile;
    LVExtractLastPathElement(codeBase);
    LVContainerRef container = _document->getContainer();
    if (!container.isNull()) {
        LVStreamRef cssStream = container->OpenStream(cssFile.c_str(), LVOM_READ);
        if (!cssStream.isNull()) {
            lString32 css;
            css << LVReadTextFile(cssStream);
            int offset = _inProgress.add(cssFile);
            ret = Parse(codeBase, css) || ret;
            _inProgress.erase(offset, 1);
        }
    }
    return ret;
}

bool LVImportStylesheetParser::Parse(lString32 codeBase, lString32 css) {
    bool ret = false;
    if (css.empty())
        return ret;
    lString8 css8 = UnicodeToUtf8(css);
    const char* s = css8.c_str();

    _nestingLevel += 1;
    while (_nestingLevel < 11) { //arbitrary limit
        lString8 import_file;

        if (LVProcessStyleSheetImport(s, import_file)) {
            lString32 importFilename = LVCombinePaths(codeBase, Utf8ToUnicode(import_file));
            if (!importFilename.empty() && !_inProgress.contains(importFilename)) {
                ret = Parse(importFilename) || ret;
            }
        } else {
            break;
        }
    }
    _nestingLevel -= 1;
    return (_document->getStyleSheet()->parse(s, false, codeBase) || ret);
}
