/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2013 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2019 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
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
