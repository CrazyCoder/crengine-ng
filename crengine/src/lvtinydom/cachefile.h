/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2010-2013 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2018,2020 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2020 NiLuJe <ninuje@gmail.com>                          *
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

#ifndef __CACHEFILE_H_INCLUDED__
#define __CACHEFILE_H_INCLUDED__

#include <lvstring.h>
#include <lvstream.h>
#include <lvptrvec.h>
#include <lvhashtable.h>
#include <lvserialbuf.h>

#include "cachefileitem.h"
#include "cachefileheader.h"

struct CacheFileItem;

#if (USE_ZSTD == 1)
struct zstd_comp_res_t;
struct zstd_decomp_res_t;
#endif

#if (USE_ZLIB == 1)
struct zlib_res_t;
#endif

#define CACHE_FILE_WRITE_BLOCK_PADDING 1

enum CacheFileBlockType
{
    CBT_FREE = 0,
    CBT_INDEX = 1,
    CBT_TEXT_DATA,
    CBT_ELEM_DATA,
    CBT_RECT_DATA, //4
    CBT_ELEM_STYLE_DATA,
    CBT_MAPS_DATA,
    CBT_PAGE_DATA, //7
    CBT_PROP_DATA,
    CBT_NODE_INDEX,
    CBT_ELEM_NODE,
    CBT_TEXT_NODE,
    CBT_REND_PARAMS, //12
    CBT_TOC_DATA,
    CBT_PAGEMAP_DATA,
    CBT_STYLE_DATA,
    CBT_BLOB_INDEX, //16
    CBT_BLOB_DATA,
    CBT_FONT_DATA //18
};

class CacheFile
{
    int _sectorSize; // block position and size granularity
    int _size;
    bool _indexChanged;
    bool _dirty;
    lUInt32 _domVersion;
    CacheCompressionType _compType;
    lString32 _cachePath;
    LVStreamRef _stream;                          // file stream
    LVPtrVector<CacheFileItem, true> _index;      // full file block index
    LVPtrVector<CacheFileItem, false> _freeIndex; // free file block index
    LVHashTable<lUInt32, CacheFileItem*> _map;    // hash map for fast search
#if (USE_ZSTD == 1)
    zstd_comp_res_t* _zstd_comp_res;
    zstd_decomp_res_t* _zstd_decomp_res;
#endif
#if (USE_ZLIB == 1)
    zlib_res_t* _zlib_comp_res;
    zlib_res_t* _zlib_uncomp_res;
#endif
    // searches for existing block
    CacheFileItem* findBlock(lUInt16 type, lUInt16 index);
    // alocates block at index, reuses existing one, if possible
    CacheFileItem* allocBlock(lUInt16 type, lUInt16 index, int size);
    // mark block as free, for later reusing
    void freeBlock(CacheFileItem* block);
    // writes file header
    bool updateHeader();
    // writes index block
    bool writeIndex();
    // reads index from file
    bool readIndex();
    // reads all blocks of index and checks CRCs
    bool validateContents();

#if (USE_ZSTD == 1)
    bool zstdAllocComp();
    void zstdCleanComp();
    bool zstdAllocDecomp();
    void zstdCleanDecomp();
    /// pack data from buf to dstbuf (using zstd)
    bool zstdPack(const lUInt8* buf, size_t bufsize, lUInt8*& dstbuf, lUInt32& dstsize);
    /// unpack data from compbuf to dstbuf (using zstd)
    bool zstdUnpack(const lUInt8* compbuf, size_t compsize, lUInt8*& dstbuf, lUInt32& dstsize);
#endif
#if (USE_ZLIB == 1)
    bool zlibAllocCompRes();
    void zlibCompCleanup();
    bool zlibAllocUncompRes();
    void zlibUncompCleanup();
    /// pack data from buf to dstbuf (using zlib)
    bool zlibPack(const lUInt8* buf, size_t bufsize, lUInt8*& dstbuf, lUInt32& dstsize);
    /// unpack data from compbuf to dstbuf (using zlib)
    bool zlibUnpack(const lUInt8* compbuf, size_t compsize, lUInt8*& dstbuf, lUInt32& dstsize);
#endif
public:
    // return current file size
    int getSize() {
        return _size;
    }
    // create uninitialized cache file, call open or create to initialize
    CacheFile(lUInt32 domVersion, CacheCompressionType compType);
    // free resources
    ~CacheFile();
    // try open existing cache file
    bool open(lString32 filename);
    // try open existing cache file from stream
    bool open(LVStreamRef stream);
    // create new cache file
    bool create(lString32 filename);
    // create new cache file in stream
    bool create(LVStreamRef stream);
    /// writes block to file
    bool write(lUInt16 type, lUInt16 dataIndex, const lUInt8* buf, int size, bool compress);
    /// reads and allocates block in memory
    bool read(lUInt16 type, lUInt16 dataIndex, lUInt8*& buf, int& size);
    /// reads and validates block
    bool validate(CacheFileItem* block);
    /// writes content of serial buffer
    bool write(lUInt16 type, lUInt16 index, SerialBuf& buf, bool compress);
    /// reads content of serial buffer
    bool read(lUInt16 type, lUInt16 index, SerialBuf& buf);
    /// writes content of serial buffer
    bool write(lUInt16 type, SerialBuf& buf, bool compress) {
        return write(type, 0, buf, compress);
    }
    /// reads content of serial buffer
    bool read(lUInt16 type, SerialBuf& buf) {
        return read(type, 0, buf);
    }
    /// reads block as a stream
    LVStreamRef readStream(lUInt16 type, lUInt16 index);

    /// cleanup resources used by the compressor
    void cleanupCompressor();
    /// cleanup resources used by the decompressor
    void cleanupUncompressor();
    /// pack data from buf to dstbuf
    bool ldomPack(const lUInt8* buf, size_t bufsize, lUInt8*& dstbuf, lUInt32& dstsize);
    /// unpack data from compbuf to dstbuf
    bool ldomUnpack(const lUInt8* compbuf, size_t compsize, lUInt8*& dstbuf, lUInt32& dstsize);

    /// sets dirty flag value, returns true if value is changed
    bool setDirtyFlag(bool dirty);
    /// sets DOM version value, returns true if value is changed
    bool setDOMVersion(lUInt32 domVersion);
    // flushes index
    bool flush(bool clearDirtyFlag, CRTimerUtil& maxTime);
    int roundSector(int n) {
        return (n + (_sectorSize - 1)) & ~(_sectorSize - 1);
    }
    void setAutoSyncSize(int sz) {
        _stream->setAutoSyncSize(sz);
    }
    void setCachePath(const lString32 cachePath) {
        _cachePath = cachePath;
    }
    const lString32 getCachePath() {
        return _cachePath;
    }
};

/// pass true to enable CRC check for
void enableCacheFileContentsValidation(bool enable);

#endif // __CACHEFILE_H_INCLUDED__
