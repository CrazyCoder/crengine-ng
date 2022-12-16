/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007-2011 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
 *   Copyright (C) 2021 poire-z <poire-z@users.noreply.github.com>         *
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

#ifndef __LDOMELEMENTWRITER_H_INCLUDED__
#define __LDOMELEMENTWRITER_H_INCLUDED__

#include <lvstring.h>

class ldomDocument;
struct ldomNode;
class LVTocItem;
struct css_elem_def_props_t;

class ldomElementWriter
{
    ldomElementWriter* _parent;
    ldomDocument* _document;

    ldomNode* _element;
    LVTocItem* _tocItem;
    lString32 _path;
    const css_elem_def_props_t* _typeDef;
    bool _allowText;
    bool _isBlock;
    bool _isSection;
#if MATHML_SUPPORT == 1
    bool _insideMathML;
#endif
    bool _stylesheetIsSet;
    bool _bodyEnterCalled;
    int _pseudoElementAfterChildIndex;
    lUInt32 _flags;
    lUInt32 getFlags();
    void updateTocItem();
    void onBodyEnter();
    void onBodyExit();
    ldomNode* getElement() {
        return _element;
    }
    lString32 getPath();
    void onText(const lChar32* text, int len, lUInt32 flags, bool insert_before_last_child = false);
    void addAttribute(lUInt16 nsid, lUInt16 id, const lChar32* value);
    //lxmlElementWriter * pop( lUInt16 id );

    ldomElementWriter(ldomDocument* document, lUInt16 nsid, lUInt16 id, ldomElementWriter* parent, bool insert_before_last_child = false);
    ~ldomElementWriter();

    friend class ldomDocumentWriter;
    friend class ldomDocumentWriterFilter;
#if MATHML_SUPPORT == 1
    friend class MathMLHelper;
#endif
    //friend ldomElementWriter * pop( ldomElementWriter * obj, lUInt16 id );
};

#endif // __LDOMELEMENTWRITER_H_INCLUDED__
