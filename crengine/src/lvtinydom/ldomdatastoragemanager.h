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

#ifndef __LDOMDATASTORAGEMANAGER_H_INCLUDED__
#define __LDOMDATASTORAGEMANAGER_H_INCLUDED__

#include <lvptrvec.h>
#include <crtimerutil.h>
#include <lvstring.h>

class tinyNodeCollection;
class ldomTextStorageChunk;
class CacheFile;
struct TextDataStorageItem;
struct ElementDataStorageItem;
struct ldomNodeStyleInfo;
class lvdomElementFormatRec;

class ldomDataStorageManager
{
    friend class ldomTextStorageChunk;
protected:
    tinyNodeCollection* _owner;
    LVPtrVector<ldomTextStorageChunk> _chunks;
    ldomTextStorageChunk* _activeChunk;
    ldomTextStorageChunk* _recentChunk;
    CacheFile* _cache;
    lUInt32 _uncompressedSize;
    lUInt32 _maxUncompressedSize;
    lUInt32 _chunkSize;
    char _type; /// type, to show in log
    bool _maxSizeReachedWarned;
    ldomTextStorageChunk* getChunk(lUInt32 address);
public:
    /// type
    lUInt16 cacheType();
    /// saves all unsaved chunks to cache file
    bool save(CRTimerUtil& maxTime);
    /// load chunk index from cache file
    bool load();
    /// sets cache file
    void setCache(CacheFile* cache);
    /// checks buffer sizes, compacts most unused chunks
    void compact(lUInt32 reservedSpace, const ldomTextStorageChunk* excludedChunk = NULL);
    lUInt32 getUncompressedSize() {
        return _uncompressedSize;
    }
#if BUILD_LITE != 1
    /// allocates new text node, return its address inside storage
    lUInt32 allocText(lUInt32 dataIndex, lUInt32 parentIndex, const lString8& text);
    /// allocates storage for new element, returns address address inside storage
    lUInt32 allocElem(lUInt32 dataIndex, lUInt32 parentIndex, lUInt32 childCount, lUInt32 attrCount);
    /// get text by address
    lString8 getText(lUInt32 address);
    /// get pointer to text data
    TextDataStorageItem* getTextItem(lUInt32 addr);
    /// get pointer to element data
    ElementDataStorageItem* getElem(lUInt32 addr);
    /// change node's parent, returns true if modified
    bool setParent(lUInt32 address, lUInt32 parent);
    /// returns node's parent by address
    lUInt32 getParent(lUInt32 address);
    /// free data item
    void freeNode(lUInt32 addr);
    /// call to invalidate chunk if content is modified
    void modified(lUInt32 addr);
    /// return true if some chunks have been allocated
    bool hasChunks() {
        return _chunks.length() > 0;
    }
#endif

    /// get or allocate space for rect data item
    void getRendRectData(lUInt32 elemDataIndex, lvdomElementFormatRec* dst);
    /// set rect data item
    void setRendRectData(lUInt32 elemDataIndex, const lvdomElementFormatRec* src);

    /// get or allocate space for element style data item
    void getStyleData(lUInt32 elemDataIndex, ldomNodeStyleInfo* dst);
    /// set element style data item
    void setStyleData(lUInt32 elemDataIndex, const ldomNodeStyleInfo* src);

    ldomDataStorageManager(tinyNodeCollection* owner, char type, lUInt32 maxUnpackedSize, lUInt32 chunkSize);
    ~ldomDataStorageManager();
};

#endif // __LDOMDATASTORAGEMANAGER_H_INCLUDED__
