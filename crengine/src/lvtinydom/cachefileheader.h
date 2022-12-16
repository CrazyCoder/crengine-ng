/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2010,2011 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2020 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2021 ourairquality <info@ourairquality.org>             *
 *   Copyright (C) 2020,2021 Aleksey Chernov <valexlin@gmail.com>          *
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

#ifndef __CACHEFILEHEADER_H_INCLUDED__
#define __CACHEFILEHEADER_H_INCLUDED__

#include <lvtypes.h>

#include "cachefileitem.h"

#define CACHE_FILE_MAGIC_SIZE 40

struct SimpleCacheFileHeader
{
    char _magic[CACHE_FILE_MAGIC_SIZE] = { 0 }; // magic
    lUInt32 _dirty;
    lUInt32 _dom_version;
    SimpleCacheFileHeader(lUInt32 dirtyFlag, lUInt32 domVersion, CacheCompressionType comptype);
};

struct CacheFileHeader: public SimpleCacheFileHeader
{
    lUInt32 _fsize;
    // Padding to explicitly align the index block structure, and that can be
    // be initialized to zero for reproducible file contents.
    lUInt32 _padding;
    CacheFileItem _indexBlock; // index array block parameters,
    // duplicate of one of index records which contains
    bool validate(lUInt32 domVersionRequested);
    CacheCompressionType compressionType();
    CacheFileHeader();
    CacheFileHeader(CacheFileItem* indexRec, int fsize, lUInt32 dirtyFlag, lUInt32 domVersion, CacheCompressionType comptype);
};

#endif // __CACHEFILEHEADER_H_INCLUDED__
