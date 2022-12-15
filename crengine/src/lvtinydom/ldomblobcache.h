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
