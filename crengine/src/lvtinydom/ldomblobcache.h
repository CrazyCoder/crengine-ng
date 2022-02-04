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

#ifndef __LVDOMBLOBCACHE_H_INCLUDED__
#define __LVDOMBLOBCACHE_H_INCLUDED__

#include <lvptrvec.h>
#include <lvtinynodecollection.h>

#include "ldomblobitem.h"

class CacheFile;

class ldomBlobCache
{
    CacheFile* _cacheFile;
    LVPtrVector<ldomBlobItem> _list;
    bool _changed;
    bool loadIndex();
    bool saveIndex();
public:
    ldomBlobCache();
    void setCacheFile(CacheFile* cacheFile);
    ContinuousOperationResult saveToCache(CRTimerUtil& timeout);
    bool addBlob(const lUInt8* data, int size, lString32 name);
    LVStreamRef getBlob(lString32 name);
};

#endif // __LVDOMBLOBCACHE_H_INCLUDED__
