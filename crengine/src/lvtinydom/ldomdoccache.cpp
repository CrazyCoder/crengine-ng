/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2009 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2018 poire-z <poire-z@users.noreply.github.com>         *
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
