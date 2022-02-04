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

#include "cachefileheader.h"
#include "lvtinydom_private.h"

#include <string.h>
#include <stdio.h>

static const char COMPRESSED_ZLIB_CACHE_FILE_MAGIC[] = "CoolReader 3 Cache"
                                                       " File v" CACHE_FILE_FORMAT_VERSION ": "
                                                       "c0"
                                                       "m1"
                                                       "\n";

static const char COMPRESSED_ZSTD_CACHE_FILE_MAGIC[] = "CoolReader 3 Cache"
                                                       " File v" CACHE_FILE_FORMAT_VERSION ": "
                                                       "c0"
                                                       "mS"
                                                       "\n";

static const char UNCOMPRESSED_CACHE_FILE_MAGIC[] = "CoolReader 3 Cache"
                                                    " File v" CACHE_FILE_FORMAT_VERSION ": "
                                                    "c0"
                                                    "m0"
                                                    "\n";

SimpleCacheFileHeader::SimpleCacheFileHeader(lUInt32 dirtyFlag, lUInt32 domVersion, CacheCompressionType comptype) {
    switch (comptype) {
        case CacheCompressionZSTD:
            memcpy(_magic, COMPRESSED_ZSTD_CACHE_FILE_MAGIC, CACHE_FILE_MAGIC_SIZE);
            break;
        case CacheCompressionZlib:
            memcpy(_magic, COMPRESSED_ZLIB_CACHE_FILE_MAGIC, CACHE_FILE_MAGIC_SIZE);
            break;
        case CacheCompressionNone:
        default:
            memcpy(_magic, UNCOMPRESSED_CACHE_FILE_MAGIC, CACHE_FILE_MAGIC_SIZE);
            break;
    }
    _dirty = dirtyFlag;
    _dom_version = domVersion;
}

bool CacheFileHeader::validate(lUInt32 domVersionRequested) {
    bool comp_match = false;
    comp_match = memcmp(_magic, COMPRESSED_ZSTD_CACHE_FILE_MAGIC, CACHE_FILE_MAGIC_SIZE) == 0;
    if (!comp_match)
        comp_match = memcmp(_magic, COMPRESSED_ZLIB_CACHE_FILE_MAGIC, CACHE_FILE_MAGIC_SIZE) == 0;
    if (!comp_match)
        comp_match = memcmp(_magic, UNCOMPRESSED_CACHE_FILE_MAGIC, CACHE_FILE_MAGIC_SIZE) == 0;
    if (!comp_match) {
        CRLog::error("CacheFileHeader::validate: magic doesn't match");
        return false;
    }
    if (_dirty != 0) {
        CRLog::error("CacheFileHeader::validate: dirty flag is set");
        printf("CRE: ignoring cache file (marked as dirty)\n");
        return false;
    }
    if (_dom_version != domVersionRequested) {
        CRLog::error("CacheFileHeader::validate: DOM version mismatch");
        printf("CRE: ignoring cache file (dom version mismatch)\n");
        return false;
    }
    return true;
}

CacheCompressionType CacheFileHeader::compressionType() {
    if (memcmp(_magic, COMPRESSED_ZSTD_CACHE_FILE_MAGIC, CACHE_FILE_MAGIC_SIZE) == 0)
        return CacheCompressionZSTD;
    if (memcmp(_magic, COMPRESSED_ZLIB_CACHE_FILE_MAGIC, CACHE_FILE_MAGIC_SIZE) == 0)
        return CacheCompressionZlib;
    return CacheCompressionNone;
}

CacheFileHeader::CacheFileHeader()
        : SimpleCacheFileHeader(0, 0, CacheCompressionNone)
        , _fsize(0)
        , _padding(0)
        , _indexBlock(0, 0) {
    memset((void*)&_indexBlock, 0, sizeof(CacheFileItem));
}

CacheFileHeader::CacheFileHeader(CacheFileItem* indexRec, int fsize, lUInt32 dirtyFlag, lUInt32 domVersion, CacheCompressionType comptype)
        : SimpleCacheFileHeader(dirtyFlag, domVersion, comptype)
        , _padding(0)
        , _indexBlock(0, 0) {
    if (indexRec) {
        memcpy((void*)&_indexBlock, indexRec, sizeof(CacheFileItem));
    } else
        memset((void*)&_indexBlock, 0, sizeof(CacheFileItem));
    _fsize = fsize;
}
