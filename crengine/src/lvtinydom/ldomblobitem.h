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

#ifndef __LVDOMBLOBITEM_H_INCLUDED__
#define __LVDOMBLOBITEM_H_INCLUDED__

#include <lvstring.h>

#include <string.h>

// BLOB storage

class ldomBlobItem
{
    int _storageIndex;
    lString32 _name;
    int _size;
    lUInt8* _data;
public:
    ldomBlobItem(lString32 name)
            : _storageIndex(-1)
            , _name(name)
            , _size(0)
            , _data(NULL) {
    }
    ~ldomBlobItem() {
        if (_data)
            delete[] _data;
    }
    int getSize() {
        return _size;
    }
    int getIndex() {
        return _storageIndex;
    }
    lUInt8* getData() {
        return _data;
    }
    lString32 getName() {
        return _name;
    }
    void setIndex(int index, int size) {
        if (_data)
            delete[] _data;
        _data = NULL;
        _storageIndex = index;
        _size = size;
    }
    void setData(const lUInt8* data, int size) {
        if (_data)
            delete[] _data;
        if (data && size > 0) {
            _data = new lUInt8[size];
            memcpy(_data, data, size);
            _size = size;
        } else {
            _data = NULL;
            _size = -1;
        }
    }
};

#endif // __LVDOMBLOBITEM_H_INCLUDED__
