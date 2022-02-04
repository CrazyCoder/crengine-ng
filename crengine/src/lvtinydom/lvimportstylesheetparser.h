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

#ifndef __LVIMPORTSTYLESHEETPARSER_H_INCLUDED__
#define __LVIMPORTSTYLESHEETPARSER_H_INCLUDED__

#include <crengine-ng-config.h>

#if BUILD_LITE != 1

#include <lvstring32collection.h>

class ldomDocument;

class LVImportStylesheetParser
{
public:
    LVImportStylesheetParser(ldomDocument* document)
            : _document(document)
            , _nestingLevel(0) {
    }
    ~LVImportStylesheetParser() {
        _inProgress.clear();
    }
    bool Parse(lString32 cssFile);
    bool Parse(lString32 codeBase, lString32 css);
private:
    ldomDocument* _document;
    lString32Collection _inProgress;
    int _nestingLevel;
};

#endif // BUILD_LITE != 1

#endif // __LVIMPORTSTYLESHEETPARSER_H_INCLUDED__
