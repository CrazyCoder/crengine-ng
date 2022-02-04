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
#if BUILD_LITE != 1
    /// pack data, and remove unpacked, put packed data to cache file
    bool swapToCache(bool removeFromMemory);
    /// read packed data from cache
    bool restoreFromCache();
#endif
    /// unpacks chunk, if packed; checks storage space, compact if necessary
    void ensureUnpacked();
#if BUILD_LITE != 1
    /// free data item
    void freeNode(int offset);
    /// saves data to cache file, if unsaved
    bool save();
#endif
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
