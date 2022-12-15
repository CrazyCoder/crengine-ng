/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2010 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#ifndef __LDOMTEXTNODE_H_INCLUDED__
#define __LDOMTEXTNODE_H_INCLUDED__

#include <lvstring.h>

/// mutable text node
class ldomTextNode
{
    lUInt32 _parentIndex;
    lString8 _text;
public:
    lUInt32 getParentIndex() {
        return _parentIndex;
    }

    void setParentIndex(lUInt32 n) {
        _parentIndex = n;
    }

    ldomTextNode(lUInt32 parentIndex, const lString8& text)
            : _parentIndex(parentIndex)
            , _text(text) {
    }

    lString8 getText() {
        return _text;
    }

    lString32 getText32() {
        return Utf8ToUnicode(_text);
    }

    void setText(const lString8& s) {
        _text = s;
    }

    void setText(const lString32& s) {
        _text = UnicodeToUtf8(s);
    }
};

#endif // __LDOMTEXTNODE_H_INCLUDED__
