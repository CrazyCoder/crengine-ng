/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2011 Vadim Lopatin <coolreader.org@gmail.com>           *
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
