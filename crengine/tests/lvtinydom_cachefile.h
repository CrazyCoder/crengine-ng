/**
 *  crengine-ng
 *
 *  \file lvtinydom_cachefile.h
 *  \brief crengine-ng unit test module.
 *  Private class CacheFile from lvtinydom.cpp
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LVTINYDOM_CACHEFILE_H
#define LVTINYDOM_CACHEFILE_H

#include <lvtypes.h>
#include <lvstream.h>
#include <crtimerutil.h>
#include <lvtinydom.h>

// extracted from lvtinydom.cpp
// When the methods of the CacheFile class change in lvtinydom.cpp,
// please remember to update this class too
//  or
// TODO: extract full class CacheFile from file lvtinydom.cpp
// into separate one.

#ifdef __cplusplus
extern "C" {
#endif

class CacheFileItem;

#if (USE_ZSTD == 1)
struct zstd_comp_res_t;
struct zstd_decomp_res_t;
#endif

#if (USE_ZLIB == 1)
struct zlib_res_t;
#endif

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
    int getSize();
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
    bool write(lUInt16 type, SerialBuf& buf, bool compress);
    /// reads content of serial buffer
    bool read(lUInt16 type, SerialBuf& buf);
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
    int roundSector(int n);
    void setAutoSyncSize(int sz);
    void setCachePath(const lString32 cachePath);
    const lString32 getCachePath();
};

#ifdef __cplusplus
}
#endif

#endif // LVTINYDOM_CACHEFILE_H
