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

#include "ldomblobcache.h"
#include <lvserialbuf.h>
#include <lvstreamutils.h>

#include "cachefile.h"

ldomBlobCache::ldomBlobCache()
        : _cacheFile(NULL)
        , _changed(false) {
}

#define BLOB_INDEX_MAGIC "BLOBINDX"

bool ldomBlobCache::loadIndex() {
    bool res;
    SerialBuf buf(0, true);
    res = _cacheFile->read(CBT_BLOB_INDEX, buf);
    if (!res) {
        _list.clear();
        return true; // missing blob index: treat as empty list of blobs
    }
    if (!buf.checkMagic(BLOB_INDEX_MAGIC))
        return false;
    lUInt32 len;
    buf >> len;
    for (lUInt32 i = 0; i < len; i++) {
        lString32 name;
        buf >> name;
        lUInt32 size;
        buf >> size;
        if (buf.error())
            break;
        ldomBlobItem* item = new ldomBlobItem(name);
        item->setIndex(i, size);
        _list.add(item);
    }
    res = !buf.error();
    return res;
}

bool ldomBlobCache::saveIndex() {
    bool res;
    SerialBuf buf(0, true);
    buf.putMagic(BLOB_INDEX_MAGIC);
    lUInt32 len = _list.length();
    buf << len;
    for (lUInt32 i = 0; i < len; i++) {
        ldomBlobItem* item = _list[i];
        buf << item->getName();
        buf << (lUInt32)item->getSize();
    }
    res = _cacheFile->write(CBT_BLOB_INDEX, buf, false);
    return res;
}

ContinuousOperationResult ldomBlobCache::saveToCache(CRTimerUtil& timeout) {
    if (!_list.length() || !_changed || _cacheFile == NULL)
        return CR_DONE;
    bool res = true;
    for (int i = 0; i < _list.length(); i++) {
        ldomBlobItem* item = _list[i];
        if (item->getData()) {
            res = _cacheFile->write(CBT_BLOB_DATA, i, item->getData(), item->getSize(), false) && res;
            if (res)
                item->setIndex(i, item->getSize());
        }
        if (timeout.expired())
            return CR_TIMEOUT;
    }
    res = saveIndex() && res;
    if (res)
        _changed = false;
    return res ? CR_DONE : CR_ERROR;
}

void ldomBlobCache::setCacheFile(CacheFile* cacheFile) {
    _cacheFile = cacheFile;
    CRTimerUtil infinite;
    if (_list.empty())
        loadIndex();
    else
        saveToCache(infinite);
}

bool ldomBlobCache::addBlob(const lUInt8* data, int size, lString32 name) {
    CRLog::debug("ldomBlobCache::addBlob( %s, size=%d, [%02x,%02x,%02x,%02x] )", LCSTR(name), size, data[0], data[1], data[2], data[3]);
    int index = _list.length();
    ldomBlobItem* item = new ldomBlobItem(name);
    if (_cacheFile != NULL) {
        _cacheFile->write(CBT_BLOB_DATA, index, data, size, false);
        item->setIndex(index, size);
    } else {
        item->setData(data, size);
    }
    _list.add(item);
    _changed = true;
    return true;
}

LVStreamRef ldomBlobCache::getBlob(lString32 name) {
    ldomBlobItem* item = NULL;
    lUInt16 index = 0;
    for (int i = 0; i < _list.length(); i++) {
        if (_list[i]->getName() == name) {
            item = _list[i];
            index = i;
            break;
        }
    }
    if (item) {
        if (item->getData()) {
            // RAM
            return LVCreateMemoryStream(item->getData(), item->getSize(), true);
        } else {
            // CACHE FILE
            return _cacheFile->readStream(CBT_BLOB_DATA, index);
        }
    }
    return LVStreamRef();
}
