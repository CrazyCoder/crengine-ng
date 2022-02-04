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
