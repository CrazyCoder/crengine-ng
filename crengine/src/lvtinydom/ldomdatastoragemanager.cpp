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

#include "ldomdatastoragemanager.h"
#include <lvtinydom_common.h>
#include <lvtinynodecollection.h>
#include <lvstyles.h>
#include <lvserialbuf.h>
#include <crlog.h>

#include "lvtinydom_private.h"
#include "ldomtextstoragechunk.h"
#include "cachefile.h"
#include "datastorageitem.h"

#define RECT_DATA_CHUNK_ITEMS_SHIFT  11
#define STYLE_DATA_CHUNK_ITEMS_SHIFT 12

#define RECT_DATA_CHUNK_ITEMS (1 << RECT_DATA_CHUNK_ITEMS_SHIFT)
#define RECT_DATA_CHUNK_SIZE  (RECT_DATA_CHUNK_ITEMS * sizeof(lvdomElementFormatRec))
#define RECT_DATA_CHUNK_MASK  (RECT_DATA_CHUNK_ITEMS - 1)

#define STYLE_DATA_CHUNK_ITEMS (1 << STYLE_DATA_CHUNK_ITEMS_SHIFT)
#define STYLE_DATA_CHUNK_SIZE  (STYLE_DATA_CHUNK_ITEMS * sizeof(ldomNodeStyleInfo))
#define STYLE_DATA_CHUNK_MASK  (STYLE_DATA_CHUNK_ITEMS - 1)

/// saves all unsaved chunks to cache file
bool ldomDataStorageManager::save(CRTimerUtil& maxTime) {
    bool res = true;
#if BUILD_LITE != 1
    if (!_cache)
        return true;
    for (int i = 0; i < _chunks.length(); i++) {
        if (!_chunks[i]->save()) {
            res = false;
            break;
        }
        //CRLog::trace("time elapsed: %d", (int)maxTime.elapsed());
        if (maxTime.expired())
            return res;
        //        if ( (i&3)==3 &&  maxTime.expired() )
        //            return res;
    }
    if (!maxTime.infinite())
        _cache->flush(false, maxTime); // intermediate flush
    if (maxTime.expired())
        return res;
    if (!res)
        return false;
    // save chunk index
    int n = _chunks.length();
    SerialBuf buf(n * 4 + 4, true);
    buf << (lUInt32)n;
    for (int i = 0; i < n; i++) {
        buf << (lUInt32)_chunks[i]->_bufpos;
    }
    res = _cache->write(cacheType(), 0xFFFF, buf, COMPRESS_NODE_STORAGE_DATA);
    if (!res) {
        CRLog::error("ldomDataStorageManager::save() - Cannot write chunk index");
    }
#endif
    return res;
}

/// load chunk index from cache file
bool ldomDataStorageManager::load() {
#if BUILD_LITE != 1
    if (!_cache)
        return false;
    //load chunk index
    SerialBuf buf(0, true);
    if (!_cache->read(cacheType(), 0xFFFF, buf)) {
        CRLog::error("ldomDataStorageManager::load() - Cannot read chunk index");
        return false;
    }
    lUInt32 n;
    buf >> n;
    if (n > 10000)
        return false; // invalid
    _recentChunk = NULL;
    _chunks.clear();
    lUInt32 compsize = 0;
    lUInt32 uncompsize = 0;
    for (lUInt32 i = 0; i < n; i++) {
        buf >> uncompsize;
        if (buf.error()) {
            _chunks.clear();
            return false;
        }
        _chunks.add(new ldomTextStorageChunk(this, (lUInt16)i, compsize, uncompsize));
    }
    return true;
#else
    return false;
#endif
}

/// get chunk pointer and update usage data
ldomTextStorageChunk* ldomDataStorageManager::getChunk(lUInt32 address) {
    ldomTextStorageChunk* chunk = _chunks[address >> 16];
    if (chunk != _recentChunk) {
        if (chunk->_prevRecent)
            chunk->_prevRecent->_nextRecent = chunk->_nextRecent;
        if (chunk->_nextRecent)
            chunk->_nextRecent->_prevRecent = chunk->_prevRecent;
        chunk->_prevRecent = NULL;
        if (((chunk->_nextRecent = _recentChunk)))
            _recentChunk->_prevRecent = chunk;
        _recentChunk = chunk;
    }
    chunk->ensureUnpacked();
    return chunk;
}

void ldomDataStorageManager::setCache(CacheFile* cache) {
    _cache = cache;
}

/// type
lUInt16 ldomDataStorageManager::cacheType() {
    switch (_type) {
        case 't':
            return CBT_TEXT_DATA;
        case 'e':
            return CBT_ELEM_DATA;
        case 'r':
            return CBT_RECT_DATA;
        case 's':
            return CBT_ELEM_STYLE_DATA;
    }
    return 0;
}

/// get or allocate space for element style data item
void ldomDataStorageManager::getStyleData(lUInt32 elemDataIndex, ldomNodeStyleInfo* dst) {
    // assume storage has raw data chunks
    int index = elemDataIndex >> 4; // element sequential index
    int chunkIndex = index >> STYLE_DATA_CHUNK_ITEMS_SHIFT;
    while (_chunks.length() <= chunkIndex) {
        //if ( _chunks.length()>0 )
        //    _chunks[_chunks.length()-1]->compact();
        _chunks.add(new ldomTextStorageChunk(STYLE_DATA_CHUNK_SIZE, this, _chunks.length()));
        getChunk((_chunks.length() - 1) << 16);
        compact(0);
    }
    ldomTextStorageChunk* chunk = getChunk(chunkIndex << 16);
    int offsetIndex = index & STYLE_DATA_CHUNK_MASK;
    chunk->getRaw(offsetIndex * sizeof(ldomNodeStyleInfo), sizeof(ldomNodeStyleInfo), (lUInt8*)dst);
}

/// set element style data item
void ldomDataStorageManager::setStyleData(lUInt32 elemDataIndex, const ldomNodeStyleInfo* src) {
    // assume storage has raw data chunks
    int index = elemDataIndex >> 4; // element sequential index
    int chunkIndex = index >> STYLE_DATA_CHUNK_ITEMS_SHIFT;
    while (_chunks.length() <= chunkIndex) {
        //if ( _chunks.length()>0 )
        //    _chunks[_chunks.length()-1]->compact();
        _chunks.add(new ldomTextStorageChunk(STYLE_DATA_CHUNK_SIZE, this, _chunks.length()));
        getChunk((_chunks.length() - 1) << 16);
        compact(0);
    }
    ldomTextStorageChunk* chunk = getChunk(chunkIndex << 16);
    int offsetIndex = index & STYLE_DATA_CHUNK_MASK;
    chunk->setRaw(offsetIndex * sizeof(ldomNodeStyleInfo), sizeof(ldomNodeStyleInfo), (const lUInt8*)src);
}

/// get or allocate space for rect data item
void ldomDataStorageManager::getRendRectData(lUInt32 elemDataIndex, lvdomElementFormatRec* dst) {
    // assume storage has raw data chunks
    int index = elemDataIndex >> 4; // element sequential index
    int chunkIndex = index >> RECT_DATA_CHUNK_ITEMS_SHIFT;
    while (_chunks.length() <= chunkIndex) {
        //if ( _chunks.length()>0 )
        //    _chunks[_chunks.length()-1]->compact();
        _chunks.add(new ldomTextStorageChunk(RECT_DATA_CHUNK_SIZE, this, _chunks.length()));
        getChunk((_chunks.length() - 1) << 16);
        compact(0);
    }
    ldomTextStorageChunk* chunk = getChunk(chunkIndex << 16);
    int offsetIndex = index & RECT_DATA_CHUNK_MASK;
    chunk->getRaw(offsetIndex * sizeof(lvdomElementFormatRec), sizeof(lvdomElementFormatRec), (lUInt8*)dst);
}

/// set rect data item
void ldomDataStorageManager::setRendRectData(lUInt32 elemDataIndex, const lvdomElementFormatRec* src) {
    // assume storage has raw data chunks
    int index = elemDataIndex >> 4; // element sequential index
    int chunkIndex = index >> RECT_DATA_CHUNK_ITEMS_SHIFT;
    while (_chunks.length() <= chunkIndex) {
        //if ( _chunks.length()>0 )
        //    _chunks[_chunks.length()-1]->compact();
        _chunks.add(new ldomTextStorageChunk(RECT_DATA_CHUNK_SIZE, this, _chunks.length()));
        getChunk((_chunks.length() - 1) << 16);
        compact(0);
    }
    ldomTextStorageChunk* chunk = getChunk(chunkIndex << 16);
    int offsetIndex = index & RECT_DATA_CHUNK_MASK;
    chunk->setRaw(offsetIndex * sizeof(lvdomElementFormatRec), sizeof(lvdomElementFormatRec), (const lUInt8*)src);
}

#if BUILD_LITE != 1
lUInt32 ldomDataStorageManager::allocText(lUInt32 dataIndex, lUInt32 parentIndex, const lString8& text) {
    if (!_activeChunk) {
        _activeChunk = new ldomTextStorageChunk(this, _chunks.length());
        _chunks.add(_activeChunk);
        getChunk((_chunks.length() - 1) << 16);
        compact(0);
    }
    int offset = _activeChunk->addText(dataIndex, parentIndex, text);
    if (offset < 0) {
        // no space in current chunk, add one more chunk
        //_activeChunk->compact();
        _activeChunk = new ldomTextStorageChunk(this, _chunks.length());
        _chunks.add(_activeChunk);
        getChunk((_chunks.length() - 1) << 16);
        compact(0);
        offset = _activeChunk->addText(dataIndex, parentIndex, text);
        if (offset < 0)
            crFatalError(1001, "Unexpected error while allocation of text");
    }
    return offset | (_activeChunk->getIndex() << 16);
}

lUInt32 ldomDataStorageManager::allocElem(lUInt32 dataIndex, lUInt32 parentIndex, lUInt32 childCount, lUInt32 attrCount) {
    if (!_activeChunk) {
        _activeChunk = new ldomTextStorageChunk(this, _chunks.length());
        _chunks.add(_activeChunk);
        getChunk((_chunks.length() - 1) << 16);
        compact(0);
    }
    int offset = _activeChunk->addElem(dataIndex, parentIndex, childCount, attrCount);
    if (offset < 0) {
        // no space in current chunk, add one more chunk
        //_activeChunk->compact();
        _activeChunk = new ldomTextStorageChunk(this, _chunks.length());
        _chunks.add(_activeChunk);
        getChunk((_chunks.length() - 1) << 16);
        compact(0);
        offset = _activeChunk->addElem(dataIndex, parentIndex, childCount, attrCount);
        if (offset < 0)
            crFatalError(1002, "Unexpected error while allocation of element");
    }
    return offset | (_activeChunk->getIndex() << 16);
}

/// call to invalidate chunk if content is modified
void ldomDataStorageManager::modified(lUInt32 addr) {
    ldomTextStorageChunk* chunk = getChunk(addr);
    chunk->modified();
}

/// change node's parent
bool ldomDataStorageManager::setParent(lUInt32 address, lUInt32 parent) {
    ldomTextStorageChunk* chunk = getChunk(address);
    return chunk->setParent(address & 0xFFFF, parent);
}

/// free data item
void ldomDataStorageManager::freeNode(lUInt32 addr) {
    ldomTextStorageChunk* chunk = getChunk(addr);
    chunk->freeNode(addr & 0xFFFF);
}

lString8 ldomDataStorageManager::getText(lUInt32 address) {
    ldomTextStorageChunk* chunk = getChunk(address);
    return chunk->getText(address & 0xFFFF);
}

/// get pointer to element data
ElementDataStorageItem* ldomDataStorageManager::getElem(lUInt32 addr) {
    ldomTextStorageChunk* chunk = getChunk(addr);
    return chunk->getElem(addr & 0xFFFF);
}

/// returns node's parent by address
lUInt32 ldomDataStorageManager::getParent(lUInt32 addr) {
    ldomTextStorageChunk* chunk = getChunk(addr);
    return chunk->getElem(addr & 0xFFFF)->parentIndex;
}
#endif

void ldomDataStorageManager::compact(lUInt32 reservedSpace, const ldomTextStorageChunk* excludedChunk) {
#if BUILD_LITE != 1
    if (_uncompressedSize + reservedSpace > _maxUncompressedSize + _maxUncompressedSize / 10) { // allow +10% overflow
        if (!_maxSizeReachedWarned) {
            // Log once to stdout that we reached maxUncompressedSize, so we can know
            // of this fact and consider it as a possible cause for crengine bugs
            CRLog::warn("Storage for %s reached max allowed uncompressed size (%u > %u)",
                        (_type == 't' ? "TEXT NODES" : (_type == 'e' ? "ELEMENTS" : (_type == 'r' ? "RENDERED RECTS" : (_type == 's' ? "ELEMENTS' STYLE DATA" : "OTHER")))),
                        _uncompressedSize, _maxUncompressedSize);
            CRLog::warn(" -> check settings.");
            _maxSizeReachedWarned = true; // warn only once
        }
        _owner->setCacheFileStale(true); // we may write: consider cache file stale
        // do compacting
        lUInt32 sumsize = reservedSpace;
        for (ldomTextStorageChunk* p = _recentChunk; p; p = p->_nextRecent) {
            if (p->_bufsize + sumsize < _maxUncompressedSize ||
                (p == _activeChunk && reservedSpace < 0xFFFFFFF) ||
                p == excludedChunk) {
                // fits
                sumsize += p->_bufsize;
            } else {
                if (!_cache)
                    _owner->createCacheFile();
                if (_cache) {
                    if (!p->swapToCache(true)) {
                        crFatalError(111, "Swap file writing error!");
                    }
                }
            }
        }
    }
#endif
}

// max 512K of uncompressed data (~8 chunks)
#define DEF_MAX_UNCOMPRESSED_SIZE 0x80000
ldomDataStorageManager::ldomDataStorageManager(tinyNodeCollection* owner, char type, lUInt32 maxUnpackedSize, lUInt32 chunkSize)
        : _owner(owner)
        , _activeChunk(NULL)
        , _recentChunk(NULL)
        , _cache(NULL)
        , _uncompressedSize(0)
        , _maxUncompressedSize(maxUnpackedSize)
        , _chunkSize(chunkSize)
        , _type(type)
        , _maxSizeReachedWarned(false) {
}

ldomDataStorageManager::~ldomDataStorageManager() {
}
