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

#include <ldomdoccache.h>

#include <crlog.h>

#include "ldomdoccacheimpl.h"

static ldomDocCacheImpl* _cacheInstance = NULL;

bool ldomDocCache::init(lString32 cacheDir, lvsize_t maxSize) {
    if (_cacheInstance)
        delete _cacheInstance;
    CRLog::info("Initialize document cache at %s (max size = %d)", UnicodeToUtf8(cacheDir).c_str(), (int)maxSize);
    _cacheInstance = new ldomDocCacheImpl(cacheDir, maxSize);
    if (!_cacheInstance->init()) {
        delete _cacheInstance;
        _cacheInstance = NULL;
        return false;
    }
    return true;
}

bool ldomDocCache::close() {
    if (!_cacheInstance)
        return false;
    delete _cacheInstance;
    _cacheInstance = NULL;
    return true;
}

/// open existing cache file stream
LVStreamRef ldomDocCache::openExisting(lString32 filename, lUInt32 crc, lUInt32 docFlags, lString32& cachePath) {
    if (!_cacheInstance)
        return LVStreamRef();
    return _cacheInstance->openExisting(filename, crc, docFlags, cachePath);
}

/// create new cache file
LVStreamRef ldomDocCache::createNew(lString32 filename, lUInt32 crc, lUInt32 docFlags, lUInt32 fileSize, lString32& cachePath) {
    if (!_cacheInstance)
        return LVStreamRef();
    return _cacheInstance->createNew(filename, crc, docFlags, fileSize, cachePath);
}

/// delete all cache files
bool ldomDocCache::clear() {
    if (!_cacheInstance)
        return false;
    return _cacheInstance->clear();
}

/// returns true if cache is enabled (successfully initialized)
bool ldomDocCache::enabled() {
    return _cacheInstance != NULL;
}
