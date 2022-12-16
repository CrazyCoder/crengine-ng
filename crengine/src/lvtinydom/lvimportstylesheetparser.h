/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2013 Konstantin Potapov <pkbo@users.sourceforge.net>    *
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

#ifndef __LVIMPORTSTYLESHEETPARSER_H_INCLUDED__
#define __LVIMPORTSTYLESHEETPARSER_H_INCLUDED__

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

#endif // __LVIMPORTSTYLESHEETPARSER_H_INCLUDED__
