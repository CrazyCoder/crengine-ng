/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2010-2012 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#ifndef __LDOMTEXTSTORAGECHUNK_H_INCLUDED__
#define __LDOMTEXTSTORAGECHUNK_H_INCLUDED__

#include <lvstring.h>

class ldomDataStorageManager;
struct ElementDataStorageItem;

/// class to store compressed/uncompressed text nodes chunk
class ldomTextStorageChunk
{
    friend class ldomDataStorageManager;
    ldomDataStorageManager* _manager;
    ldomTextStorageChunk* _nextRecent;
    ldomTextStorageChunk* _prevRecent;
    lUInt8* _buf;     /// buffer for uncompressed data
    lUInt32 _bufsize; /// _buf (uncompressed) area size, bytes
    lUInt32 _bufpos;  /// _buf (uncompressed) data write position (for appending of new data)
    lUInt16 _index;   /// ? index of chunk in storage
    char _type;       /// type, to show in log
    bool _saved;

    void setunpacked(const lUInt8* buf, int bufsize);
    /// pack data, and remove unpacked
    void compact();
    /// pack data, and remove unpacked, put packed data to cache file
    bool swapToCache(bool removeFromMemory);
    /// read packed data from cache
    bool restoreFromCache();
    /// unpacks chunk, if packed; checks storage space, compact if necessary
    void ensureUnpacked();
    /// free data item
    void freeNode(int offset);
    /// saves data to cache file, if unsaved
    bool save();
public:
    /// call to invalidate chunk if content is modified
    void modified();
    /// returns chunk index inside collection
    int getIndex() {
        return _index;
    }
    /// returns free space in buffer
    int space();
    /// adds new text item to buffer, returns offset inside chunk of stored data
    int addText(lUInt32 dataIndex, lUInt32 parentIndex, const lString8& text);
    /// adds new element item to buffer, returns offset inside chunk of stored data
    int addElem(lUInt32 dataIndex, lUInt32 parentIndex, lUInt32 childCount, lUInt32 attrCount);
    /// get text item from buffer by offset
    lString8 getText(int offset);
    /// get node parent by offset
    lUInt32 getParent(int offset);
    /// set node parent by offset
    bool setParent(int offset, lUInt32 parentIndex);
    /// get pointer to element data
    ElementDataStorageItem* getElem(int offset);
    /// get raw data bytes
    void getRaw(int offset, int size, lUInt8* buf);
    /// set raw data bytes
    void setRaw(int offset, int size, const lUInt8* buf);
    /// create empty buffer
    ldomTextStorageChunk(ldomDataStorageManager* manager, lUInt16 index);
    /// create chunk to be read from cache file
    ldomTextStorageChunk(ldomDataStorageManager* manager, lUInt16 index, lUInt32 compsize, lUInt32 uncompsize);
    /// create with preallocated buffer, for raw access
    ldomTextStorageChunk(lUInt32 preAllocSize, ldomDataStorageManager* manager, lUInt16 index);
    ~ldomTextStorageChunk();
};

#endif // __LDOMTEXTSTORAGECHUNK_H_INCLUDED__
