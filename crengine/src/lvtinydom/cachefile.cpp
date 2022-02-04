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

#include "cachefile.h"

#include <lvstreamutils.h>
#include <crlog.h>

#include "lvtinydom_private.h"
#include "../lvstream/lvstreamfragment.h"

#if (USE_ZSTD == 1)
#include <zstd.h>
#endif
#if (USE_ZLIB == 1)
#include <zlib.h>
#endif

#define XXH_INLINE_ALL
#include "../xxhash.h"

#if (USE_ZLIB == 1)
#define PACK_BUF_SIZE   0x10000
#define UNPACK_BUF_SIZE 0x40000
#endif

/// set to 1 to enable crc check of all blocks of cache file on open
#ifndef ENABLE_CACHE_FILE_CONTENTS_VALIDATION
#define ENABLE_CACHE_FILE_CONTENTS_VALIDATION 1
#endif

#ifndef DOC_DATA_COMPRESSION_LEVEL
/// data compression level (0=no compression, 1=fast compressions, 3=normal compression)
// Note: keep this above 1, toggling between compression and no-compression
// can be done at run time by calling compressCachedData(false)
#define DOC_DATA_COMPRESSION_LEVEL 1 // 0, 1, 3 (0=no compression)
#endif

/**
 * Cache file implementation.
 */
#if (USE_ZSTD == 1)
struct zstd_comp_res_t
{
    void* buffOut;
    size_t buffOutSize;
    ZSTD_CCtx* cctx;
};
struct zstd_decomp_res_t
{
    void* buffOut;
    size_t buffOutSize;
    ZSTD_DCtx* dctx;
};
#endif

#if (USE_ZLIB == 1)
struct zlib_res_t
{
    size_t buffSize;
    z_stream zstream;
    Bytef buff[1];
};
#endif

static bool _enableCacheFileContentsValidation = (bool)ENABLE_CACHE_FILE_CONTENTS_VALIDATION;
void enableCacheFileContentsValidation(bool enable) {
    _enableCacheFileContentsValidation = enable;
}

//static lUInt32 calcHash32( const lUInt8 * s, int len )
//{
//    lUInt32 res = 0;
//    for ( int i=0; i<len; i++ ) {
//        // res*31 + s
//        res = (((((((res<<1)+res)<<1)+res)<<1)+res)<<1)+res + s[i];
//    }
//    return res;
//}

// FNV 64bit hash function
// from http://isthe.com/chongo/tech/comp/fnv/#gcc-O3

//#define NO_FNV_GCC_OPTIMIZATION
/*#define FNV_64_PRIME ((lUInt64)0x100000001b3ULL)
static lUInt64 calcHash64( const lUInt8 * s, int len )
{
    const lUInt8 * endp = s + len;
    // 64 bit FNV hash function
    lUInt64 hval = 14695981039346656037ULL;
    for ( ; s<endp; s++ ) {
#if defined(NO_FNV_GCC_OPTIMIZATION)
        hval *= FNV_64_PRIME;
#else */
/* NO_FNV_GCC_OPTIMIZATION */ /*
        hval += (hval << 1) + (hval << 4) + (hval << 5) +
            (hval << 7) + (hval << 8) + (hval << 40);
#endif */
/* NO_FNV_GCC_OPTIMIZATION */ /*
        hval ^= *s;
    }
    return hval;
}*/
static lUInt32 calcHash(const lUInt8* s, int len) {
    return XXH32(s, len, 0);
}

#define LASSERT(x) \
    if (!(x))      \
    crFatalError(1111, "assertion failed: " #x)

// create uninitialized cache file, call open or create to initialize
CacheFile::CacheFile(lUInt32 domVersion, CacheCompressionType compType)
        : _sectorSize(CACHE_FILE_SECTOR_SIZE)
        , _size(0)
        , _indexChanged(false)
        , _dirty(true)
        , _domVersion(domVersion)
        , _compType(compType)
        , _map(1024)
        , _cachePath(lString32::empty_str)
#if (USE_ZSTD == 1)
        , _zstd_comp_res(nullptr)
        , _zstd_decomp_res(nullptr)
#endif
#if (USE_ZLIB == 1)
        , _zlib_comp_res(nullptr)
        , _zlib_uncomp_res(nullptr)
#endif
{
}

// free resources
CacheFile::~CacheFile() {
    if (!_stream.isNull()) {
        // don't flush -- leave file dirty
        //CRTimerUtil infinite;
        //flush( true, infinite );
    }
#if (USE_ZSTD == 1)
    zstdCleanComp();
    zstdCleanDecomp();
#endif
#if (USE_ZLIB == 1)
    zlibCompCleanup();
    zlibUncompCleanup();
#endif
}

/// sets dirty flag value, returns true if value is changed
bool CacheFile::setDirtyFlag(bool dirty) {
    if (_dirty == dirty)
        return false;
    if (!dirty) {
        CRLog::info("CacheFile::clearing Dirty flag");
        _stream->Flush(true);
    } else {
        CRLog::info("CacheFile::setting Dirty flag");
    }
    _dirty = dirty;
    SimpleCacheFileHeader hdr(_dirty ? 1 : 0, _domVersion, _compType);
    _stream->SetPos(0);
    lvsize_t bytesWritten = 0;
    _stream->Write(&hdr, sizeof(hdr), &bytesWritten);
    if (bytesWritten != sizeof(hdr))
        return false;
    _stream->Flush(true);
    //CRLog::trace("setDirtyFlag : hdr is saved with Dirty flag = %d", hdr._dirty);
    return true;
}

bool CacheFile::setDOMVersion(lUInt32 domVersion) {
    if (_domVersion == domVersion)
        return false;
    CRLog::info("CacheFile::setting DOM version value");
    _domVersion = domVersion;
    SimpleCacheFileHeader hdr(_dirty ? 1 : 0, _domVersion, _compType);
    _stream->SetPos(0);
    lvsize_t bytesWritten = 0;
    _stream->Write(&hdr, sizeof(hdr), &bytesWritten);
    if (bytesWritten != sizeof(hdr))
        return false;
    _stream->Flush(true);
    //CRLog::trace("setDOMVersion : hdr is saved with DOM version = %u", hdr._domVersionRequested);
    return true;
}

// flushes index
bool CacheFile::flush(bool clearDirtyFlag, CRTimerUtil& maxTime) {
    if (clearDirtyFlag) {
        //setDirtyFlag(true);
        if (!writeIndex())
            return false;
        setDirtyFlag(false);
    } else {
        _stream->Flush(false, maxTime);
        //CRLog::trace("CacheFile->flush() took %d ms ", (int)timer.elapsed());
    }
    return true;
}

// reads all blocks of index and checks CRCs
bool CacheFile::validateContents() {
    CRLog::info("Started validation of cache file contents");
    LVHashTable<lUInt32, CacheFileItem*>::pair* pair;
    for (LVHashTable<lUInt32, CacheFileItem*>::iterator p = _map.forwardIterator(); (pair = p.next()) != NULL;) {
        if (pair->value->_dataType == CBT_INDEX)
            continue;
        if (!validate(pair->value)) {
            CRLog::error("Contents validation is failed for block type=%d index=%d", (int)pair->value->_dataType, pair->value->_dataIndex);
            return false;
        }
    }
    CRLog::info("Finished validation of cache file contents -- successful");
    return true;
}

// reads index from file
bool CacheFile::readIndex() {
    CacheFileHeader hdr;
    _stream->SetPos(0);
    lvsize_t bytesRead = 0;
    _stream->Read(&hdr, sizeof(hdr), &bytesRead);
    if (bytesRead != sizeof(hdr))
        return false;
    CRLog::info("Header read: DirtyFlag=%d", hdr._dirty);
    CRLog::info("Header read: DOM level=%u", hdr._dom_version);
    CRLog::info("Header read: compression type=%u", (int)hdr.compressionType());
    if (!hdr.validate(_domVersion))
        return false;
    if ((int)hdr._fsize > _size + 4096 - 1) {
        CRLog::error("CacheFile::readIndex: file size doesn't match with header");
        return false;
    }
    if (hdr.compressionType() != _compType) {
        CRLog::error("CacheFile::readIndex: compression type does not match the target");
        return false;
    }
    if (!hdr._indexBlock._blockFilePos)
        return true; // empty index is ok
    if (hdr._indexBlock._blockFilePos >= (int)hdr._fsize || hdr._indexBlock._blockFilePos + hdr._indexBlock._blockSize > (int)hdr._fsize + 4096 - 1) {
        CRLog::error("CacheFile::readIndex: Wrong index file position specified in header");
        return false;
    }
    if ((int)_stream->SetPos(hdr._indexBlock._blockFilePos) != hdr._indexBlock._blockFilePos) {
        CRLog::error("CacheFile::readIndex: cannot move file position to index block");
        return false;
    }
    int count = hdr._indexBlock._dataSize / sizeof(CacheFileItem);
    if (count < 0 || count > 100000) {
        CRLog::error("CacheFile::readIndex: invalid number of blocks in index");
        return false;
    }
    CacheFileItem* index = new CacheFileItem[count];
    bytesRead = 0;
    lvsize_t sz = sizeof(CacheFileItem) * count;
    _stream->Read(index, sz, &bytesRead);
    if (bytesRead != sz)
        return false;
    // check CRC
    lUInt32 hash = calcHash((lUInt8*)index, sz);
    if (hdr._indexBlock._dataHash != hash) {
        CRLog::error("CacheFile::readIndex: CRC doesn't match found %08x expected %08x", hash, hdr._indexBlock._dataHash);
        delete[] index;
        return false;
    }
    for (int i = 0; i < count; i++) {
        if (index[i]._dataType == CBT_INDEX)
            index[i] = hdr._indexBlock;
        if (!index[i].validate(_size)) {
            delete[] index;
            return false;
        }
        CacheFileItem* item = new CacheFileItem();
        memcpy(item, &index[i], sizeof(CacheFileItem));
        _index.add(item);
        lUInt32 key = ((lUInt32)item->_dataType) << 16 | item->_dataIndex;
        if (key == 0)
            _freeIndex.add(item);
        else
            _map.set(key, item);
    }
    delete[] index;
    CacheFileItem* indexitem = findBlock(CBT_INDEX, 0);
    if (!indexitem) {
        CRLog::error("CacheFile::readIndex: index block info doesn't match header");
        return false;
    }
    _dirty = hdr._dirty ? true : false;
    return true;
}

// writes index block
bool CacheFile::writeIndex() {
    if (!_indexChanged)
        return true; // no changes: no writes

    if (_index.length() == 0)
        return updateHeader();

    // create copy of index in memory
    int count = _index.length();
    CacheFileItem* indexItem = findBlock(CBT_INDEX, 0);
    if (!indexItem) {
        int sz = sizeof(CacheFileItem) * (count * 2 + 100);
        allocBlock(CBT_INDEX, 0, sz);
        indexItem = findBlock(CBT_INDEX, 0);
        (void)indexItem; // silences clang warning
        count = _index.length();
    }
    CacheFileItem* index = new CacheFileItem[count]();
    int sz = count * sizeof(CacheFileItem);
    for (int i = 0; i < count; i++) {
        memcpy(&index[i], _index[i], sizeof(CacheFileItem));
        if (index[i]._dataType == CBT_INDEX) {
            index[i]._dataHash = 0;
            index[i]._packedHash = 0;
            index[i]._dataSize = 0;
        }
    }
    bool res = write(CBT_INDEX, 0, (const lUInt8*)index, sz, false);
    delete[] index;

    indexItem = findBlock(CBT_INDEX, 0);
    if (!res || !indexItem) {
        CRLog::error("CacheFile::writeIndex: error while writing index!!!");
        return false;
    }

    updateHeader();
    _indexChanged = false;
    return true;
}

// writes file header
bool CacheFile::updateHeader() {
    CacheFileItem* indexItem = NULL;
    indexItem = findBlock(CBT_INDEX, 0);
    CacheFileHeader hdr(indexItem, _size, _dirty ? 1 : 0, _domVersion, _compType);
    _stream->SetPos(0);
    lvsize_t bytesWritten = 0;
    _stream->Write(&hdr, sizeof(hdr), &bytesWritten);
    if (bytesWritten != sizeof(hdr))
        return false;
    //CRLog::trace("updateHeader finished: Dirty flag = %d", hdr._dirty);
    return true;
}

//
void CacheFile::freeBlock(CacheFileItem* block) {
    lUInt32 key = ((lUInt32)block->_dataType) << 16 | block->_dataIndex;
    _map.remove(key);
    block->_dataIndex = 0;
    block->_dataType = 0;
    block->_dataSize = 0;
    _freeIndex.add(block);
}

/// reads block as a stream
LVStreamRef CacheFile::readStream(lUInt16 type, lUInt16 index) {
    CacheFileItem* block = findBlock(type, index);
    if (block && block->_dataSize) {
#if 0
        lUInt8 * buf = NULL;
        int size = 0;
        if (read(type, index, buf, size))
            return LVCreateMemoryStream(buf, size);
#else
        return LVStreamRef(new LVStreamFragment(_stream, block->_blockFilePos, block->_dataSize));
#endif
    }
    return LVStreamRef();
}

// searches for existing block
CacheFileItem* CacheFile::findBlock(lUInt16 type, lUInt16 index) {
    lUInt32 key = ((lUInt32)type) << 16 | index;
    CacheFileItem* existing = _map.get(key);
    return existing;
}

// allocates index record for block, sets its new size
CacheFileItem* CacheFile::allocBlock(lUInt16 type, lUInt16 index, int size) {
    lUInt32 key = ((lUInt32)type) << 16 | index;
    CacheFileItem* existing = _map.get(key);
    if (existing) {
        if (existing->_blockSize >= size) {
            if (existing->_dataSize != size) {
                existing->_dataSize = size;
                _indexChanged = true;
            }
            return existing;
        }
        // old block has not enough space: free it
        freeBlock(existing);
        existing = NULL;
    }
    // search for existing free block of proper size
    int bestSize = -1;
    //int bestIndex = -1;
    for (int i = 0; i < _freeIndex.length(); i++) {
        if (_freeIndex[i] && (_freeIndex[i]->_blockSize >= size) && (bestSize == -1 || _freeIndex[i]->_blockSize < bestSize)) {
            bestSize = _freeIndex[i]->_blockSize;
            //bestIndex = -1;
            existing = _freeIndex[i];
        }
    }
    if (existing) {
        _freeIndex.remove(existing);
        existing->_dataType = type;
        existing->_dataIndex = index;
        existing->_dataSize = size;
        _map.set(key, existing);
        _indexChanged = true;
        return existing;
    }
    // allocate new block
    CacheFileItem* block = new CacheFileItem(type, index);
    _map.set(key, block);
    block->_blockSize = roundSector(size);
    block->_dataSize = size;
    block->_blockIndex = _index.length();
    _index.add(block);
    block->_blockFilePos = _size;
    _size += block->_blockSize;
    _indexChanged = true;
    // really, file size is not extended
    return block;
}

/// reads and validates block
bool CacheFile::validate(CacheFileItem* block) {
    lUInt8* buf = NULL;
    unsigned size = 0;

    if ((int)_stream->SetPos(block->_blockFilePos) != block->_blockFilePos) {
        CRLog::error("CacheFile::validate: Cannot set position for block %d:%d of size %d", block->_dataType, block->_dataIndex, (int)size);
        return false;
    }

    // read block from file
    size = block->_dataSize;
    buf = (lUInt8*)malloc(size);
    lvsize_t bytesRead = 0;
    _stream->Read(buf, size, &bytesRead);
    if (bytesRead != size) {
        CRLog::error("CacheFile::validate: Cannot read block %d:%d of size %d", block->_dataType, block->_dataIndex, (int)size);
        free(buf);
        return false;
    }

    // check CRC for file block
    lUInt32 packedhash = calcHash(buf, size);
    if (packedhash != block->_packedHash) {
        CRLog::error("CacheFile::validate: packed data CRC doesn't match for block %d:%d of size %d", block->_dataType, block->_dataIndex, (int)size);
        free(buf);
        return false;
    }
    free(buf);
    return true;
}

// reads and allocates block in memory
bool CacheFile::read(lUInt16 type, lUInt16 dataIndex, lUInt8*& buf, int& size) {
    buf = NULL;
    size = 0;
    CacheFileItem* block = findBlock(type, dataIndex);
    if (!block) {
        CRLog::error("CacheFile::read: Block %d:%d not found in file", type, dataIndex);
        return false;
    }
    if ((int)_stream->SetPos(block->_blockFilePos) != block->_blockFilePos)
        return false;

    // read block from file
    size = block->_dataSize;
    buf = (lUInt8*)malloc(size);
    lvsize_t bytesRead = 0;
    _stream->Read(buf, size, &bytesRead);
    if ((int)bytesRead != size) {
        CRLog::error("CacheFile::read: Cannot read block %d:%d of size %d, bytesRead=%d", type, dataIndex, (int)size, (int)bytesRead);
        free(buf);
        buf = NULL;
        size = 0;
        return false;
    }

    bool compressed = block->_uncompressedSize != 0;

    if (compressed) {
        // block is compressed

        // check crc separately only for compressed data
        lUInt32 packedhash = calcHash(buf, size);
        if (packedhash != block->_packedHash) {
            CRLog::error("CacheFile::read: packed data CRC doesn't match for block %d:%d of size %d", type, dataIndex, (int)size);
            free(buf);
            buf = NULL;
            size = 0;
            return false;
        }

        // uncompress block data
        lUInt8* uncomp_buf = NULL;
        lUInt32 uncomp_size = 0;
        if (ldomUnpack(buf, size, uncomp_buf, uncomp_size) && uncomp_size == block->_uncompressedSize) {
            free(buf);
            buf = uncomp_buf;
            size = uncomp_size;
        } else {
            CRLog::error("CacheFile::read: error while uncompressing data for block %d:%d of size %d", type, dataIndex, (int)size);
            free(buf);
            buf = NULL;
            size = 0;
            return false;
        }
    }

    // check CRC
    lUInt32 hash = calcHash(buf, size);
    if (hash != block->_dataHash) {
        CRLog::error("CacheFile::read: CRC doesn't match for block %d:%d of size %d", type, dataIndex, (int)size);
        free(buf);
        buf = NULL;
        size = 0;
        return false;
    }
    // Success. Don't forget to free allocated block externally
    return true;
}

// writes block to file
bool CacheFile::write(lUInt16 type, lUInt16 dataIndex, const lUInt8* buf, int size, bool compress) {
    // check whether data is changed
    lUInt32 newhash = calcHash(buf, size);
    CacheFileItem* existingblock = findBlock(type, dataIndex);

    if (existingblock) {
        bool sameSize = ((int)existingblock->_uncompressedSize == size) || (existingblock->_uncompressedSize == 0 && (int)existingblock->_dataSize == size);
        if (sameSize && existingblock->_dataHash == newhash) {
            return true;
        }
    }

#if 0
    if (existingblock)
        CRLog::trace("*    oldsz=%d oldhash=%08x", (int)existingblock->_uncompressedSize, (int)existingblock->_dataHash);
    CRLog::trace("* wr block t=%d[%d] sz=%d hash=%08x", type, dataIndex, size, newhash);
#endif
    setDirtyFlag(true);

    lUInt32 uncompressedSize = 0;
    lUInt64 newpackedhash = newhash;
    if (_compType == CacheCompressionNone)
        compress = false;
    if (compress) {
        lUInt8* dstbuf = NULL;
        lUInt32 dstsize = 0;
        if (!ldomPack(buf, size, dstbuf, dstsize)) {
            compress = false;
        } else {
            uncompressedSize = size;
            size = dstsize;
            buf = dstbuf;
            newpackedhash = calcHash(buf, size);
#if DEBUG_DOM_STORAGE == 1
            //CRLog::trace("packed block %d:%d : %d to %d bytes (%d%%)", type, dataIndex, srcsize, dstsize, srcsize>0?(100*dstsize/srcsize):0 );
#endif
        }
    }

    CacheFileItem* block = NULL;
    if (existingblock && existingblock->_dataSize >= size) {
        // reuse existing block
        block = existingblock;
    } else {
        // allocate new block
        if (existingblock)
            freeBlock(existingblock);
        block = allocBlock(type, dataIndex, size);
    }
    if (!block) {
#if DOC_DATA_COMPRESSION_LEVEL != 0
        if (compress) {
            free((void*)buf);
        }
#endif
        return false;
    }
    if ((int)_stream->SetPos(block->_blockFilePos) != block->_blockFilePos) {
#if DOC_DATA_COMPRESSION_LEVEL != 0
        if (compress) {
            free((void*)buf);
        }
#endif
        return false;
    }
    // assert: size == block->_dataSize
    // actual writing of data
    block->_dataSize = size;
    lvsize_t bytesWritten = 0;
    _stream->Write(buf, size, &bytesWritten);
    if ((int)bytesWritten != size) {
#if DOC_DATA_COMPRESSION_LEVEL != 0
        if (compress) {
            free((void*)buf);
        }
#endif
        return false;
    }
#if CACHE_FILE_WRITE_BLOCK_PADDING == 1
    int paddingSize = block->_blockSize - size; //roundSector( size ) - size
    if (paddingSize) {
        if ((int)block->_blockFilePos + (int)block->_dataSize >= (int)_stream->GetSize() - _sectorSize) {
            LASSERT(size + paddingSize == block->_blockSize);
            //            if (paddingSize > 16384) {
            //                CRLog::error("paddingSize > 16384");
            //            }
            //            LASSERT(paddingSize <= 16384);
            lUInt8 tmp[16384]; //paddingSize];
            memset(tmp, 0xFF, paddingSize < 16384 ? paddingSize : 16384);
            do {
                int blkSize = paddingSize < 16384 ? paddingSize : 16384;
                _stream->Write(tmp, blkSize, &bytesWritten);
                paddingSize -= blkSize;
            } while (paddingSize > 0);
        }
    }
#endif
    //_stream->Flush(true);
    // update CRC
    block->_dataHash = newhash;
    block->_packedHash = newpackedhash;
    block->_uncompressedSize = uncompressedSize;

    if (compress) {
        free((void*)buf);
    }
    _indexChanged = true;

    //CRLog::error("CacheFile::write: block %d:%d (pos %ds, size %ds) is written (crc=%08x)", type, dataIndex, (int)block->_blockFilePos/_sectorSize, (int)(size+_sectorSize-1)/_sectorSize, block->_dataCRC);
    // success
    return true;
}

/// writes content of serial buffer
bool CacheFile::write(lUInt16 type, lUInt16 index, SerialBuf& buf, bool compress) {
    return write(type, index, buf.buf(), buf.pos(), compress);
}

/// reads content of serial buffer
bool CacheFile::read(lUInt16 type, lUInt16 index, SerialBuf& buf) {
    lUInt8* tmp = NULL;
    int size = 0;
    bool res = read(type, index, tmp, size);
    if (res) {
        buf.set(tmp, size);
    }
    buf.setPos(0);
    return res;
}

// try open existing cache file
bool CacheFile::open(lString32 filename) {
    LVStreamRef stream = LVOpenFileStream(filename.c_str(), LVOM_APPEND);
    if (!stream) {
        CRLog::error("CacheFile::open: cannot open file %s", LCSTR(filename));
        return false;
    }
    crSetFileToRemoveOnFatalError(LCSTR(filename));
    return open(stream);
}

// try open existing cache file
bool CacheFile::open(LVStreamRef stream) {
    _stream = stream;
    _size = _stream->GetSize();
    //_stream->setAutoSyncSize(STREAM_AUTO_SYNC_SIZE);

    if (!readIndex()) {
        CRLog::error("CacheFile::open : cannot read index from file");
        return false;
    }
    if (_enableCacheFileContentsValidation && !validateContents()) {
        CRLog::error("CacheFile::open : file contents validation failed");
        return false;
    }
    return true;
}

bool CacheFile::create(lString32 filename) {
    LVStreamRef stream = LVOpenFileStream(filename.c_str(), LVOM_APPEND);
    if (_stream.isNull()) {
        CRLog::error("CacheFile::create: cannot create file %s", LCSTR(filename));
        return false;
    }
    crSetFileToRemoveOnFatalError(LCSTR(filename));
    return create(stream);
}

bool CacheFile::create(LVStreamRef stream) {
    _stream = stream;
    //_stream->setAutoSyncSize(STREAM_AUTO_SYNC_SIZE);
    if (_stream->SetPos(0) != 0) {
        CRLog::error("CacheFile::create: cannot seek file");
        _stream.Clear();
        return false;
    }

    _size = _sectorSize;
    LVArray<lUInt8> sector0(_sectorSize, 0);
    lvsize_t bytesWritten = 0;
    _stream->Write(sector0.get(), _sectorSize, &bytesWritten);
    if ((int)bytesWritten != _sectorSize) {
        _stream.Clear();
        return false;
    }
    if (!updateHeader()) {
        _stream.Clear();
        return false;
    }
    return true;
}

#if (USE_ZSTD == 1)
bool CacheFile::zstdAllocComp() {
    // printf("zstdtag: CacheFile::zstdAllocComp\n");
    _zstd_comp_res = (zstd_comp_res_t*)malloc(sizeof(zstd_comp_res_t));
    if (!_zstd_comp_res)
        return false;

    _zstd_comp_res->buffOutSize = ZSTD_CStreamOutSize();
    _zstd_comp_res->buffOut = malloc(_zstd_comp_res->buffOutSize);
    if (!_zstd_comp_res->buffOut) {
        free(_zstd_comp_res);
        _zstd_comp_res = nullptr;
        return false;
    }
    _zstd_comp_res->cctx = ZSTD_createCCtx();
    if (_zstd_comp_res->cctx == nullptr) {
        free(_zstd_comp_res->buffOut);
        free(_zstd_comp_res);
        _zstd_comp_res = nullptr;
        return false;
    }

    // Parameters are sticky
    // NOTE: ZSTD_CLEVEL_DEFAULT is currently 3, sane range is 1-19
    ZSTD_CCtx_setParameter(_zstd_comp_res->cctx, ZSTD_c_compressionLevel, ZSTD_CLEVEL_DEFAULT);
    // This would be redundant with CRe's own calcHash, AFAICT?
    //ZSTD_CCtx_setParameter(_comp_ress->cctx, ZSTD_c_checksumFlag, 1);

    // Threading? (Requires libzstd built w/ threading support)
    // NOTE: Since we always use ZSTD_e_end, which basically defers to ZSTD_compress2(), this will *not* make it async,
    //       it'll still block.
    //ZSTD_CCtx_setParameter(_comp_ress->cctx, ZSTD_c_nbWorkers, 4);

    return true;
}

void CacheFile::zstdCleanComp() {
    // printf("zstdtag: CacheFile::zstdCleanComp\n");
    if (_zstd_comp_res) {
        if (_zstd_comp_res->cctx)
            ZSTD_freeCCtx(_zstd_comp_res->cctx);
        if (_zstd_comp_res->buffOut)
            free(_zstd_comp_res->buffOut);
        free(_zstd_comp_res);
        _zstd_comp_res = nullptr;
    }
}

/// pack data from buf to dstbuf (using zstd)
bool CacheFile::zstdPack(const lUInt8* buf, size_t bufsize, lUInt8*& dstbuf, lUInt32& dstsize) {
    // printf("zstdtag: zstdPack() <- %p (%zu)\n", buf, bufsize);

    // Lazy init our resources, and keep 'em around
    if (!_zstd_comp_res) {
        if (!zstdAllocComp()) {
            CRLog::error("zstdPack() failed to allocate resources");
            return false;
        }
    }

    // c.f., ZSTD's examples/streaming_compression.c
    // NOTE: We could probably gain much by training zstd and using a dictionary, here ;).
    size_t const buffOutSize = _zstd_comp_res->buffOutSize;
    void* const buffOut = _zstd_comp_res->buffOut;
    ZSTD_CCtx* const cctx = _zstd_comp_res->cctx;

    // Reset the context
    size_t const err = ZSTD_CCtx_reset(cctx, ZSTD_reset_session_only);
    if (ZSTD_isError(err)) {
        CRLog::error("ZSTD_CCtx_reset() error: %s", ZSTD_getErrorName(err));
        return false;
    }

    // Tell the compressor just how much data we need to compress
    ZSTD_CCtx_setPledgedSrcSize(cctx, bufsize);

    // Debug: compare current buffOutSize against the worst-case
    // printf("zstdtag: ZSTD_compressBound(): %zu\n", ZSTD_compressBound(bufsize));

    size_t compressed_size = 0;
    lUInt8* compressed_buf = NULL;

    ZSTD_EndDirective const mode = ZSTD_e_end;
    ZSTD_inBuffer input;
    ZSTD_outBuffer output;
    input.src = buf;
    input.size = bufsize;
    input.pos = 0;
    int finished = 0;
    do {
        output.dst = buffOut;
        output.size = buffOutSize;
        output.pos = 0;
        size_t const remaining = ZSTD_compressStream2(cctx, &output, &input, mode);
        if (ZSTD_isError(remaining)) {
            CRLog::error("zstdtag: ZSTD_compressStream2() error: %s (%zu -> %zu)", ZSTD_getErrorName(remaining), bufsize, compressed_size);
            if (compressed_buf) {
                free(compressed_buf);
            }
            return false;
        }

        compressed_buf = cr_realloc(compressed_buf, compressed_size + output.pos);
        memcpy(compressed_buf + compressed_size, buffOut, output.pos);
        compressed_size += output.pos;

        finished = (remaining == 0);
        // printf("zstdtag: zstdPack(): finished? %d (current chunk: %zu/%zu; total in: %zu; total out: %zu)\n", finished, output.pos, output.size, bufsize, compressed_size);
    } while (!finished);

    dstsize = compressed_size;
    dstbuf = compressed_buf;
    // printf("zstdtag: zstdPack() done: %zu -> %zu\n", bufsize, compressed_size);
    return true;
}

bool CacheFile::zstdAllocDecomp() {
    // printf("zstdtag: CacheFile::zstdAllocDecomp\n");
    _zstd_decomp_res = (zstd_decomp_res_t*)malloc(sizeof(zstd_decomp_res_t));
    if (!_zstd_decomp_res)
        return false;

    _zstd_decomp_res->buffOutSize = ZSTD_DStreamOutSize();
    _zstd_decomp_res->buffOut = malloc(_zstd_decomp_res->buffOutSize);
    if (!_zstd_decomp_res->buffOut) {
        free(_zstd_decomp_res);
        _zstd_decomp_res = nullptr;
        return false;
    }
    _zstd_decomp_res->dctx = ZSTD_createDCtx();
    if (_zstd_decomp_res->dctx == nullptr) {
        free(_zstd_decomp_res->buffOut);
        free(_zstd_decomp_res);
        _zstd_decomp_res = nullptr;
        return false;
    }

    return true;
}

void CacheFile::zstdCleanDecomp() {
    // printf("zstdtag: CacheFile::zstdCleanDecomp\n");
    if (_zstd_decomp_res) {
        if (_zstd_decomp_res->dctx)
            ZSTD_freeDCtx(_zstd_decomp_res->dctx);
        if (_zstd_decomp_res->buffOut)
            free(_zstd_decomp_res->buffOut);
        free(_zstd_decomp_res);
        _zstd_decomp_res = nullptr;
    }
}

/// unpack data from compbuf to dstbuf (using zstd)
bool CacheFile::zstdUnpack(const lUInt8* compbuf, size_t compsize, lUInt8*& dstbuf, lUInt32& dstsize) {
    // printf("zstdtag: zstdUnpack() <- %p (%zu)\n", compbuf, compsize);

    // Lazy init our resources, and keep 'em around
    if (!_zstd_decomp_res) {
        if (!zstdAllocDecomp()) {
            CRLog::error("zstdUnpack() failed to allocate resources");
            return false;
        }
    }

    // c.f., ZSTD's examples/streaming_decompression.c
    size_t const buffOutSize = _zstd_decomp_res->buffOutSize;
    void* const buffOut = _zstd_decomp_res->buffOut;
    ZSTD_DCtx* const dctx = _zstd_decomp_res->dctx;

    // Reset the context
    size_t const err = ZSTD_DCtx_reset(dctx, ZSTD_reset_session_only);
    if (ZSTD_isError(err)) {
        CRLog::error("ZSTD_DCtx_reset() error: %s", ZSTD_getErrorName(err));
        return false;
    }

    size_t uncompressed_size = 0;
    lUInt8* uncompressed_buf = NULL;

    size_t lastRet = 0;
    ZSTD_inBuffer input;
    ZSTD_outBuffer output;
    input.src = compbuf;
    input.size = compsize;
    input.pos = 0;
    while (input.pos < input.size) {
        output.dst = buffOut;
        output.size = buffOutSize;
        output.pos = 0;
        size_t const ret = ZSTD_decompressStream(dctx, &output, &input);
        if (ZSTD_isError(ret)) {
            CRLog::error("zstdtag: ZSTD_decompressStream() error: %s (%zu -> %zu)", ZSTD_getErrorName(ret), compsize, uncompressed_size);
            if (uncompressed_buf) {
                free(uncompressed_buf);
            }
            return false;
        }

        uncompressed_buf = cr_realloc(uncompressed_buf, uncompressed_size + output.pos);
        memcpy(uncompressed_buf + uncompressed_size, buffOut, output.pos);
        uncompressed_size += output.pos;

        lastRet = ret;
        // printf("zstdtag: zstdUnpack(): ret: %zu (current chunk: %zu/%zu)\n", ret, output.pos, output.size);
    }

    if (lastRet != 0) {
        CRLog::error("zstdtag: zstdUnpack(): EOF before end of stream: %zu", lastRet);
        if (uncompressed_buf) {
            free(uncompressed_buf);
        }
        return false;
    }

    dstsize = uncompressed_size;
    dstbuf = uncompressed_buf;
    // printf("zstdtag: zstdUnpack() done: %zu -> %zu\n", compsize, uncompressed_size);
    return true;
}
#endif // (USE_ZSTD==1)

#if (USE_ZLIB == 1)
bool CacheFile::zlibAllocCompRes() {
    // printf("zlibtag: CacheFile::zlibAllocCompRes\n");
    if (_zlib_comp_res)
        return true;
    _zlib_comp_res = (zlib_res_t*)malloc(sizeof(zlib_res_t) + PACK_BUF_SIZE - 1);
    if (!_zlib_comp_res)
        return false;
    _zlib_comp_res->buffSize = PACK_BUF_SIZE;
    z_streamp z = &_zlib_comp_res->zstream;
    z->zalloc = Z_NULL;
    z->zfree = Z_NULL;
    z->opaque = Z_NULL;
    int ret = deflateInit(z, DOC_DATA_COMPRESSION_LEVEL);
    if (ret != Z_OK) {
        free(_zlib_comp_res);
        _zlib_comp_res = NULL;
        return false;
    }
    return true;
}

void CacheFile::zlibCompCleanup() {
    // printf("zlibtag: CacheFile::zlibCompCleanup\n");
    if (_zlib_comp_res) {
        zlib_res_t* res = (zlib_res_t*)_zlib_comp_res;
        deflateEnd(&res->zstream);
        free(_zlib_comp_res);
        _zlib_comp_res = NULL;
    }
}

bool CacheFile::zlibAllocUncompRes() {
    // printf("zlibtag: CacheFile::zlibAllocUncompRes\n");
    if (_zlib_uncomp_res)
        return true;
    _zlib_uncomp_res = (zlib_res_t*)malloc(sizeof(zlib_res_t) + UNPACK_BUF_SIZE - 1);
    if (!_zlib_uncomp_res)
        return false;
    _zlib_uncomp_res->buffSize = UNPACK_BUF_SIZE;
    z_streamp z = &_zlib_uncomp_res->zstream;
    z->zalloc = Z_NULL;
    z->zfree = Z_NULL;
    z->opaque = Z_NULL;
    int ret = inflateInit(z);
    if (ret != Z_OK) {
        free(_zlib_uncomp_res);
        _zlib_uncomp_res = NULL;
        return false;
    }
    return true;
}

void CacheFile::zlibUncompCleanup() {
    // printf("zlibtag: CacheFile::zlibUncompCleanup\n");
    if (_zlib_uncomp_res) {
        zlib_res_t* res = (zlib_res_t*)_zlib_uncomp_res;
        inflateEnd(&res->zstream);
        free(_zlib_uncomp_res);
        _zlib_uncomp_res = NULL;
    }
}

/// pack data from buf to dstbuf (using zlib)
bool CacheFile::zlibPack(const lUInt8* buf, size_t bufsize, lUInt8*& dstbuf, lUInt32& dstsize) {
    // printf("zlibtag: zlibPack() <- %p (%zu)\n", buf, bufsize);

    // Lazy init our resources, and keep 'em around
    if (!_zlib_comp_res) {
        if (!zlibAllocCompRes()) {
            CRLog::error("zlibtag: zlibPack() failed to allocate resources");
            return false;
        }
    }

    int ret;
    z_streamp z = &_zlib_comp_res->zstream;
    ret = deflateReset(z);
    if (ret != Z_OK) {
        CRLog::error("zlibtag: deflateReset() error: %d", ret);
        return false;
    }
    z->avail_in = bufsize;
    z->next_in = (unsigned char*)buf;
    int compressed_size = 0;
    lUInt8* compressed_buf = NULL;
    do {
        z->avail_out = _zlib_comp_res->buffSize;
        z->next_out = &_zlib_comp_res->buff[0];
        ret = deflate(z, Z_FINISH);
        if (ret == Z_STREAM_ERROR) { // some error occured while packing
            deflateEnd(z);
            if (compressed_buf)
                free(compressed_buf);
            // printf("zlibtag: deflate() error: %d (%d > %d)\n", ret, bufsize, compressed_size);
            return false;
        }
        int have = _zlib_comp_res->buffSize - z->avail_out;
        compressed_buf = cr_realloc(compressed_buf, compressed_size + have);
        memcpy(compressed_buf + compressed_size, &_zlib_comp_res->buff[0], have);
        compressed_size += have;
        // printf("zlibtag: deflate() additional call needed (%d > %d)\n", bufsize, compressed_size);
    } while (z->avail_out == 0); // buffer fully filled => deflate in progress
    dstsize = compressed_size;
    dstbuf = compressed_buf;
    // printf("zlibtag: deflate() done: %d > %d\n", bufsize, compressed_size);
    return true;
}

/// unpack data from compbuf to dstbuf (using zlib)
bool CacheFile::zlibUnpack(const lUInt8* compbuf, size_t compsize, lUInt8*& dstbuf, lUInt32& dstsize) {
    // printf("zlibtag: zlibUnpack() <- %p (%zu)\n", compbuf, compsize);

    // Lazy init our resources, and keep 'em around
    if (!_zlib_uncomp_res) {
        if (!zlibAllocUncompRes()) {
            CRLog::error("zlibtag: zlibUnpack() failed to allocate resources");
            return false;
        }
    }

    int ret;
    z_streamp z = &_zlib_uncomp_res->zstream;
    ret = inflateReset(z);
    if (ret != Z_OK) {
        CRLog::error("zlibtag: inflateReset() error: %d", ret);
        return false;
    }

    z->avail_in = compsize;
    z->next_in = (unsigned char*)compbuf;
    lUInt32 uncompressed_size = 0;
    lUInt8* uncompressed_buf = NULL;
    do {
        z->avail_out = _zlib_uncomp_res->buffSize;
        z->next_out = &_zlib_uncomp_res->buff[0];
        ret = inflate(z, Z_SYNC_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) { // some error occured while unpacking
            inflateEnd(z);
            if (uncompressed_buf)
                free(uncompressed_buf);
            dstbuf = NULL;
            dstsize = 0;
            // printf("zlibtag: inflate() error: %d (%d > %d)\n", ret, compsize, uncompressed_size);
            return false;
        }
        lUInt32 have = _zlib_uncomp_res->buffSize - z->avail_out;
        uncompressed_buf = cr_realloc(uncompressed_buf, uncompressed_size + have);
        memcpy(uncompressed_buf + uncompressed_size, &_zlib_uncomp_res->buff[0], have);
        uncompressed_size += have;
        // printf("zlibtag: inflate() additional call needed (%d > %d)\n", compsize, uncompressed_size);
    } while (ret != Z_STREAM_END);
    dstsize = uncompressed_size;
    dstbuf = uncompressed_buf;
    // printf("zlibtag: inflate() done %d > %d\n", compsize, uncompressed_size);
    return true;
}
#endif // (USE_ZLIB==1)

void CacheFile::cleanupCompressor() {
    switch (_compType) {
        case CacheCompressionZSTD:
#if (USE_ZSTD == 1)
            zstdCleanComp();
#endif
            break;
        case CacheCompressionZlib:
#if (USE_ZLIB == 1)
            zlibCompCleanup();
#endif
            break;
        case CacheCompressionNone:
            break;
    }
}

void CacheFile::cleanupUncompressor() {
    switch (_compType) {
        case CacheCompressionZSTD:
#if (USE_ZSTD == 1)
            zstdCleanDecomp();
#endif
            break;
        case CacheCompressionZlib:
#if (USE_ZLIB == 1)
            zlibUncompCleanup();
#endif
            break;
        case CacheCompressionNone:
            break;
    }
}

/// pack data from buf to dstbuf
bool CacheFile::ldomPack(const lUInt8* buf, size_t bufsize, lUInt8*& dstbuf, lUInt32& dstsize) {
    switch (_compType) {
        case CacheCompressionZSTD:
#if (USE_ZSTD == 1)
            return zstdPack(buf, bufsize, dstbuf, dstsize);
#endif
            break;
        case CacheCompressionZlib:
#if (USE_ZLIB == 1)
            return zlibPack(buf, bufsize, dstbuf, dstsize);
#endif
            break;
        case CacheCompressionNone:
        default:
            return false;
    }
    return false;
}

/// unpack data from compbuf to dstbuf
bool CacheFile::ldomUnpack(const lUInt8* compbuf, size_t compsize, lUInt8*& dstbuf, lUInt32& dstsize) {
    switch (_compType) {
        case CacheCompressionZSTD:
#if (USE_ZSTD == 1)
            return zstdUnpack(compbuf, compsize, dstbuf, dstsize);
#endif
            break;
        case CacheCompressionZlib:
#if (USE_ZLIB == 1)
            return zlibUnpack(compbuf, compsize, dstbuf, dstsize);
#endif
            break;
        case CacheCompressionNone:
            break;
    }
    return false;
}
