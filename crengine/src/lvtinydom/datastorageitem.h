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
