/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007,2009,2011 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2018 poire-z <poire-z@users.noreply.github.com>         *
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

#ifndef __LDOMATTRIBUTECOLLECTION_H_INCLUDED__
#define __LDOMATTRIBUTECOLLECTION_H_INCLUDED__

#include "lxmlattribute.h"

class ldomAttributeCollection
{
private:
    lUInt16 _len;
    lUInt16 _size;
    lxmlAttribute* _list;
public:
    ldomAttributeCollection()
            : _len(0)
            , _size(0)
            , _list(NULL) {
    }
    ~ldomAttributeCollection() {
        if (_list)
            free(_list);
    }
    lxmlAttribute* operator[](int index) {
        return &_list[index];
    }
    const lxmlAttribute* operator[](int index) const {
        return &_list[index];
    }
    lUInt16 length() const {
        return _len;
    }
    lUInt32 get(lUInt16 nsId, lUInt16 attrId) const {
        for (lUInt16 i = 0; i < _len; i++) {
            if (_list[i].compare(nsId, attrId))
                return _list[i].index;
        }
        return LXML_ATTR_VALUE_NONE;
    }
    void set(lUInt16 nsId, lUInt16 attrId, lUInt32 valueIndex) {
        // find existing
        for (lUInt16 i = 0; i < _len; i++) {
            if (_list[i].compare(nsId, attrId)) {
                _list[i].index = valueIndex;
                return;
            }
        }
        // add
        if (_len >= _size) {
            _size += 4;
            _list = cr_realloc(_list, _size);
        }
        _list[_len++].setData(nsId, attrId, valueIndex);
    }
    void add(lUInt16 nsId, lUInt16 attrId, lUInt32 valueIndex) {
        // find existing
        if (_len >= _size) {
            _size += 4;
            _list = cr_realloc(_list, _size);
        }
        _list[_len++].setData(nsId, attrId, valueIndex);
    }
    void add(const lxmlAttribute* v) {
        // find existing
        if (_len >= _size) {
            _size += 4;
            _list = cr_realloc(_list, _size);
        }
        _list[_len++] = *v;
    }
};

#endif // __LDOMATTRIBUTECOLLECTION_H_INCLUDED__
