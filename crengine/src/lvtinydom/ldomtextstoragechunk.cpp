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

#include "ldomtextstoragechunk.h"

#include <crlog.h>

#include "lvtinydom_private.h"
#include "ldomdatastoragemanager.h"
#include "cachefile.h"
#include "datastorageitem.h"

/// create chunk to be read from cache file
ldomTextStorageChunk::ldomTextStorageChunk(ldomDataStorageManager* manager, lUInt16 index, lUInt32 compsize, lUInt32 uncompsize)
        : _manager(manager)
        , _nextRecent(NULL)
        , _prevRecent(NULL)
        , _buf(NULL)          /// buffer for uncompressed data
        , _bufsize(0)         /// _buf (uncompressed) area size, bytes
        , _bufpos(uncompsize) /// _buf (uncompressed) data write position (for appending of new data)
        , _index(index)       /// ? index of chunk in storage
        , _type(manager->_type)
        , _saved(true) {
    CR_UNUSED(compsize);
}

ldomTextStorageChunk::ldomTextStorageChunk(lUInt32 preAllocSize, ldomDataStorageManager* manager, lUInt16 index)
        : _manager(manager)
        , _nextRecent(NULL)
        , _prevRecent(NULL)
        , _buf(NULL)             /// buffer for uncompressed data
        , _bufsize(preAllocSize) /// _buf (uncompressed) area size, bytes
        , _bufpos(preAllocSize)  /// _buf (uncompressed) data write position (for appending of new data)
        , _index(index)          /// ? index of chunk in storage
        , _type(manager->_type)
        , _saved(false) {
    _buf = (lUInt8*)calloc(preAllocSize, sizeof(*_buf));
    _manager->_uncompressedSize += _bufsize;
}

ldomTextStorageChunk::ldomTextStorageChunk(ldomDataStorageManager* manager, lUInt16 index)
        : _manager(manager)
        , _nextRecent(NULL)
        , _prevRecent(NULL)
        , _buf(NULL)    /// buffer for uncompressed data
        , _bufsize(0)   /// _buf (uncompressed) area size, bytes
        , _bufpos(0)    /// _buf (uncompressed) data write position (for appending of new data)
        , _index(index) /// ? index of chunk in storage
        , _type(manager->_type)
        , _saved(false) {
}

/// saves data to cache file, if unsaved
bool ldomTextStorageChunk::save() {
    if (!_saved)
        return swapToCache(false);
    return true;
}

ldomTextStorageChunk::~ldomTextStorageChunk() {
    setunpacked(NULL, 0);
}

/// pack data, and remove unpacked, put packed data to cache file
bool ldomTextStorageChunk::swapToCache(bool removeFromMemory) {
    if (!_manager->_cache)
        return true;
    if (_buf) {
        if (!_saved && _manager->_cache) {
#if DEBUG_DOM_STORAGE == 1
            CRLog::debug("Writing %d bytes of chunk %c%d to cache", _bufpos, _type, _index);
#endif
            if (!_manager->_cache->write(_manager->cacheType(), _index, _buf, _bufpos, COMPRESS_NODE_STORAGE_DATA)) {
                CRLog::error("Error while swapping of chunk %c%d to cache file", _type, _index);
                crFatalError(-1, "Error while swapping of chunk to cache file");
                return false;
            }
            _saved = true;
        }
    }
    if (removeFromMemory) {
        setunpacked(NULL, 0);
    }
    return true;
}

/// read packed data from cache
bool ldomTextStorageChunk::restoreFromCache() {
    if (_buf)
        return true;
    if (!_saved)
        return false;
    int size;
    if (!_manager->_cache->read(_manager->cacheType(), _index, _buf, size))
        return false;
    _bufsize = size;
    _manager->_uncompressedSize += _bufsize;
#if DEBUG_DOM_STORAGE == 1
    CRLog::debug("Read %d bytes of chunk %c%d from cache", _bufsize, _type, _index);
#endif
    return true;
}

/// get raw data bytes
void ldomTextStorageChunk::getRaw(int offset, int size, lUInt8* buf) {
#ifdef _DEBUG
    if (!_buf || offset + size > (int)_bufpos || offset + size > (int)_bufsize)
        crFatalError(123, "ldomTextStorageChunk: Invalid raw data buffer position");
#endif
    memcpy(buf, _buf + offset, size);
}

/// set raw data bytes
void ldomTextStorageChunk::setRaw(int offset, int size, const lUInt8* buf) {
#ifdef _DEBUG
    if (!_buf || offset + size > (int)_bufpos || offset + size > (int)_bufsize)
        crFatalError(123, "ldomTextStorageChunk: Invalid raw data buffer position");
#endif
    if (memcmp(_buf + offset, buf, size) != 0) {
        memcpy(_buf + offset, buf, size);
        modified();
    }
}

/// returns free space in buffer
int ldomTextStorageChunk::space() {
    return _bufsize - _bufpos;
}

/// returns free space in buffer
int ldomTextStorageChunk::addText(lUInt32 dataIndex, lUInt32 parentIndex, const lString8& text) {
    lUInt32 itemsize = (sizeof(TextDataStorageItem) + text.length() - 2 + 15) & 0xFFFFFFF0;
    if (!_buf) {
        // create new buffer, if necessary
        _bufsize = _manager->_chunkSize > itemsize ? _manager->_chunkSize : itemsize;
        _buf = (lUInt8*)calloc(_bufsize, sizeof(*_buf));
        _bufpos = 0;
        _manager->_uncompressedSize += _bufsize;
    }
    if (_bufsize - _bufpos < itemsize)
        return -1;
    TextDataStorageItem* p = (TextDataStorageItem*)(_buf + _bufpos);
    p->sizeDiv16 = (lUInt16)(itemsize >> 4);
    p->dataIndex = dataIndex;
    p->parentIndex = parentIndex;
    p->type = LXML_TEXT_NODE;
    p->length = (lUInt16)text.length();
    memcpy(p->text, text.c_str(), p->length);
    int res = _bufpos >> 4;
    _bufpos += itemsize;
    return res;
}

/// adds new element item to buffer, returns offset inside chunk of stored data
int ldomTextStorageChunk::addElem(lUInt32 dataIndex, lUInt32 parentIndex, lUInt32 childCount, lUInt32 attrCount) {
    lUInt32 itemsize = (sizeof(ElementDataStorageItem) + attrCount * (sizeof(lUInt16) * 2 + sizeof(lUInt32)) + childCount * sizeof(lUInt32) - sizeof(lUInt32) + 15) & 0xFFFFFFF0;
    if (!_buf) {
        // create new buffer, if necessary
        _bufsize = _manager->_chunkSize > itemsize ? _manager->_chunkSize : itemsize;
        _buf = (lUInt8*)calloc(_bufsize, sizeof(*_buf));
        _bufpos = 0;
        _manager->_uncompressedSize += _bufsize;
    }
    if (_bufsize - _bufpos < itemsize)
        return -1;
    ElementDataStorageItem* item = (ElementDataStorageItem*)(_buf + _bufpos);
    if (item) {
        item->sizeDiv16 = (lUInt16)(itemsize >> 4);
        item->dataIndex = dataIndex;
        item->parentIndex = parentIndex;
        item->type = LXML_ELEMENT_NODE;
        item->parentIndex = parentIndex;
        item->attrCount = (lUInt16)attrCount;
        item->childCount = childCount;
    }
    int res = _bufpos >> 4;
    _bufpos += itemsize;
    return res;
}

/// set node parent by offset
bool ldomTextStorageChunk::setParent(int offset, lUInt32 parentIndex) {
    offset <<= 4;
    if (offset >= 0 && offset < (int)_bufpos) {
        TextDataStorageItem* item = (TextDataStorageItem*)(_buf + offset);
        if ((int)parentIndex != item->parentIndex) {
            item->parentIndex = parentIndex;
            modified();
            return true;
        } else
            return false;
    }
    CRLog::error("Offset %d is out of bounds (%d) for storage chunk %c%d, chunkCount=%d", offset, this->_bufpos, this->_type, this->_index, _manager->_chunks.length());
    return false;
}

/// get text node parent by offset
lUInt32 ldomTextStorageChunk::getParent(int offset) {
    offset <<= 4;
    if (offset >= 0 && offset < (int)_bufpos) {
        TextDataStorageItem* item = (TextDataStorageItem*)(_buf + offset);
        return item->parentIndex;
    }
    CRLog::error("Offset %d is out of bounds (%d) for storage chunk %c%d, chunkCount=%d", offset, this->_bufpos, this->_type, this->_index, _manager->_chunks.length());
    return 0;
}

/// get pointer to element data
ElementDataStorageItem* ldomTextStorageChunk::getElem(int offset) {
    offset <<= 4;
    if (offset >= 0 && offset < (int)_bufpos) {
        ElementDataStorageItem* item = (ElementDataStorageItem*)(_buf + offset);
        return item;
    }
    CRLog::error("Offset %d is out of bounds (%d) for storage chunk %c%d, chunkCount=%d", offset, this->_bufpos, this->_type, this->_index, _manager->_chunks.length());
    return NULL;
}

/// call to invalidate chunk if content is modified
void ldomTextStorageChunk::modified() {
    if (!_buf) {
        CRLog::error("Modified is called for node which is not in memory");
    }
    _saved = false;
}

/// free data item
void ldomTextStorageChunk::freeNode(int offset) {
    offset <<= 4;
    if (_buf && offset >= 0 && offset < (int)_bufpos) {
        TextDataStorageItem* item = (TextDataStorageItem*)(_buf + offset);
        if ((item->type == LXML_TEXT_NODE || item->type == LXML_ELEMENT_NODE) && item->dataIndex) {
            item->type = LXML_NO_DATA;
            item->dataIndex = 0;
            modified();
        }
    }
}

/// get text item from buffer by offset
lString8 ldomTextStorageChunk::getText(int offset) {
    offset <<= 4;
    if (_buf && offset >= 0 && offset < (int)_bufpos) {
        TextDataStorageItem* item = (TextDataStorageItem*)(_buf + offset);
        return item->getText8();
    }
    return lString8::empty_str;
}

void ldomTextStorageChunk::setunpacked(const lUInt8* buf, int bufsize) {
    if (_buf) {
        _manager->_uncompressedSize -= _bufsize;
        free(_buf);
        _buf = NULL;
        _bufsize = 0;
    }
    if (buf && bufsize) {
        _bufsize = bufsize;
        _bufpos = bufsize;
        _buf = (lUInt8*)malloc(sizeof(lUInt8) * bufsize);
        _manager->_uncompressedSize += _bufsize;
        memcpy(_buf, buf, bufsize);
    }
}

/// unpacks chunk, if packed; checks storage space, compact if necessary
void ldomTextStorageChunk::ensureUnpacked() {
    if (!_buf) {
        if (_saved) {
            if (!restoreFromCache()) {
                CRTimerUtil timer;
                timer.infinite();
                _manager->_cache->flush(false, timer);
                CRLog::warn("restoreFromCache() failed for chunk %c%d, will try after flush", _type, _index);
                if (!restoreFromCache()) {
                    CRLog::error("restoreFromCache() failed for chunk %c%d", _type, _index);
                    crFatalError(111, "restoreFromCache() failed for chunk");
                }
            }
            _manager->compact(0, this);
        }
    } else {
        // compact
    }
}
