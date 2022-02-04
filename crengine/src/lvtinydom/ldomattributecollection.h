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
