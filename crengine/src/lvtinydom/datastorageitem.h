/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2009,2010 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2018 poire-z <poire-z@users.noreply.github.com>         *
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

#ifndef __DATASTORAGEITEM_H_INCLUDED__
#define __DATASTORAGEITEM_H_INCLUDED__

#include "lxmlattribute.h"

/// common header for data storage items
struct DataStorageItemHeader
{
    /// item type: LXML_TEXT_NODE, LXML_ELEMENT_NODE, LXML_NO_DATA
    lUInt16 type;
    /// size of item / 16
    lUInt16 sizeDiv16;
    /// data index of this node in document
    lInt32 dataIndex;
    /// data index of parent node in document, 0 means no parent
    lInt32 parentIndex;
};

/// text node storage implementation
struct TextDataStorageItem: public DataStorageItemHeader
{
    /// utf8 text length, characters
    lUInt16 length;
    /// utf8 text, w/o zero
    lChar8 text[2]; // utf8 text follows here, w/o zero byte at end
    /// return text
    inline lString32 getText() {
        return Utf8ToUnicode(text, length);
    }
    inline lString8 getText8() {
        return lString8(text, length);
    }
};

/// element node data storage
struct ElementDataStorageItem: public DataStorageItemHeader
{
    lUInt16 id;
    lUInt16 nsid;
    lInt16 attrCount;
    lUInt8 rendMethod;
    lUInt8 reserved8;
    lInt32 childCount;
    lInt32 children[1];
    lUInt16* attrs() {
        return (lUInt16*)(children + childCount);
    }
    lxmlAttribute* attr(int index) {
        return (lxmlAttribute*)&(((lUInt16*)(children + childCount))[index * 4]);
    }
    lUInt32 getAttrValueId(lUInt16 ns, lUInt16 id) {
        lUInt16* a = attrs();
        for (int i = 0; i < attrCount; i++) {
            lxmlAttribute* attr = (lxmlAttribute*)(&a[i * 4]);
            if (!attr->compare(ns, id))
                continue;
            return attr->index;
        }
        return LXML_ATTR_VALUE_NONE;
    }
    lxmlAttribute* findAttr(lUInt16 ns, lUInt16 id) {
        lUInt16* a = attrs();
        for (int i = 0; i < attrCount; i++) {
            lxmlAttribute* attr = (lxmlAttribute*)(&a[i * 4]);
            if (attr->compare(ns, id))
                return attr;
        }
        return NULL;
    }
    // TODO: add items here
    //css_style_ref_t _style;
    //font_ref_t      _font;
};

#endif // __DATASTORAGEITEM_H_INCLUDED__
