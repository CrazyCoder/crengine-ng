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

#include "ldomdoccacheimpl.h"

#include <lvstreamutils.h>
#include <lvserialbuf.h>
#include <lvcontaineriteminfo.h>
#include <crlog.h>

#include <stdio.h>

#include "lvtinydom_private.h"
#include "../lvstream/lvstreambuffer.h"

//--------------------------------------------------------
// cache memory sizes
//--------------------------------------------------------
#ifndef ENABLED_BLOCK_WRITE_CACHE
#define ENABLED_BLOCK_WRITE_CACHE 1
#endif

#define WRITE_CACHE_TOTAL_SIZE (10 * DOC_BUFFER_SIZE / 100)

// calculated parameters
#define WRITE_CACHE_BLOCK_SIZE  0x4000
#define WRITE_CACHE_BLOCK_COUNT (WRITE_CACHE_TOTAL_SIZE / WRITE_CACHE_BLOCK_SIZE)
#define TEST_BLOCK_STREAM       0

static const char* doccache_magic = "CoolReader3 Document Cache Directory Index\nV1.00\n";

ldomDocCacheImpl::ldomDocCacheImpl(lString32 cacheDir, lvsize_t maxSize)
        : _cacheDir(cacheDir)
        , _maxSize(maxSize)
        , _oldStreamSize(0)
        , _oldStreamCRC(0) {
    LVAppendPathDelimiter(_cacheDir);
    CRLog::trace("ldomDocCacheImpl(%s maxSize=%d)", LCSTR(_cacheDir), (int)maxSize);
}

bool ldomDocCacheImpl::writeIndex() {
    lString32 filename = _cacheDir + "cr3cache.inx";
    if (_oldStreamSize == 0) {
        LVStreamRef oldStream = LVOpenFileStream(filename.c_str(), LVOM_READ);
        if (!oldStream.isNull()) {
            _oldStreamSize = (lUInt32)oldStream->GetSize();
            _oldStreamCRC = (lUInt32)oldStream->getcrc32();
        }
    }

    // fill buffer
    SerialBuf buf(16384, true);
    buf.putMagic(doccache_magic);
    lUInt32 start = buf.pos();
    int count = _files.length();
    buf << (lUInt32)count;
    for (int i = 0; i < count && !buf.error(); i++) {
        FileItem* item = _files[i];
        buf << item->filename;
        buf << item->size;
        CRLog::trace("cache item: %s %d", LCSTR(item->filename), (int)item->size);
    }
    buf.putCRC(buf.pos() - start);
    if (buf.error())
        return false;
    lUInt32 newCRC = buf.getCRC();
    lUInt32 newSize = buf.pos();

    // check to avoid rewritting of identical file
    if (newCRC != _oldStreamCRC || newSize != _oldStreamSize) {
        // changed: need to write
        CRLog::trace("Writing cache index");
        LVStreamRef stream = LVOpenFileStream(filename.c_str(), LVOM_WRITE);
        if (!stream)
            return false;
        if (stream->Write(buf.buf(), buf.pos(), NULL) != LVERR_OK)
            return false;
        _oldStreamCRC = newCRC;
        _oldStreamSize = newSize;
    }
    return true;
}

bool ldomDocCacheImpl::readIndex() {
    lString32 filename = _cacheDir + "cr3cache.inx";
    // read index
    lUInt32 totalSize = 0;
    LVStreamRef instream = LVOpenFileStream(filename.c_str(), LVOM_READ);
    if (!instream.isNull()) {
        LVStreamBufferRef sb = instream->GetReadBuffer(0, instream->GetSize());
        if (!sb)
            return false;
        SerialBuf buf(sb->getReadOnly(), sb->getSize());
        if (!buf.checkMagic(doccache_magic)) {
            CRLog::error("wrong cache index file format");
            return false;
        }

        lUInt32 start = buf.pos();
        lUInt32 count;
        buf >> count;
        for (lUInt32 i = 0; i < count && !buf.error(); i++) {
            FileItem* item = new FileItem();
            _files.add(item);
            buf >> item->filename;
            buf >> item->size;
            CRLog::trace("cache %d: %s [%d]", i, UnicodeToUtf8(item->filename).c_str(), (int)item->size);
            totalSize += item->size;
        }
        if (!buf.checkCRC(buf.pos() - start)) {
            CRLog::error("CRC32 doesn't match in cache index file");
            return false;
        }

        if (buf.error())
            return false;

        CRLog::info("Document cache index file read ok, %d files in cache, %d bytes", _files.length(), totalSize);
        return true;
    } else {
        CRLog::error("Document cache index file cannot be read");
        return false;
    }
}

bool ldomDocCacheImpl::removeExtraFiles() {
    LVContainerRef container;
    container = LVOpenDirectory(_cacheDir.c_str(), U"*.cr3");
    if (container.isNull()) {
        if (!LVCreateDirectory(_cacheDir)) {
            CRLog::error("Cannot create directory %s", UnicodeToUtf8(_cacheDir).c_str());
            return false;
        }
        container = LVOpenDirectory(_cacheDir.c_str(), U"*.cr3");
        if (container.isNull()) {
            CRLog::error("Cannot open directory %s", UnicodeToUtf8(_cacheDir).c_str());
            return false;
        }
    }
    for (int i = 0; i < container->GetObjectCount(); i++) {
        const LVContainerItemInfo* item = container->GetObjectInfo(i);
        if (!item->IsContainer()) {
            lString32 fn = item->GetName();
            if (!fn.endsWith(".cr3"))
                continue;
            if (findFileIndex(fn) < 0) {
                // delete file
                CRLog::info("Removing cache file not specified in index: %s", UnicodeToUtf8(fn).c_str());
                if (!LVDeleteFile(_cacheDir + fn)) {
                    CRLog::error("Error while removing cache file not specified in index: %s", UnicodeToUtf8(fn).c_str());
                }
            }
        }
    }
    return true;
}

bool ldomDocCacheImpl::reserve(lvsize_t allocSize) {
    bool res = true;
    // remove extra files specified in list
    lvsize_t dirsize = allocSize;
    for (int i = 0; i < _files.length();) {
        if (LVFileExists(_cacheDir + _files[i]->filename)) {
            if ((i > 0 || allocSize > 0) && dirsize + _files[i]->size > _maxSize) {
                if (LVDeleteFile(_cacheDir + _files[i]->filename)) {
                    _files.erase(i, 1);
                } else {
                    CRLog::error("Cannot delete cache file %s", UnicodeToUtf8(_files[i]->filename).c_str());
                    dirsize += _files[i]->size;
                    res = false;
                    i++;
                }
            } else {
                dirsize += _files[i]->size;
                i++;
            }
        } else {
            CRLog::error("File %s is found in cache index, but does not exist", UnicodeToUtf8(_files[i]->filename).c_str());
            _files.erase(i, 1);
        }
    }
    return res;
}

int ldomDocCacheImpl::findFileIndex(lString32 filename) {
    for (int i = 0; i < _files.length(); i++) {
        if (_files[i]->filename == filename)
            return i;
    }
    return -1;
}

bool ldomDocCacheImpl::moveFileToTop(lString32 filename, lUInt32 size) {
    int index = findFileIndex(filename);
    if (index < 0) {
        FileItem* item = new FileItem();
        item->filename = filename;
        item->size = size;
        _files.insert(0, item);
    } else {
        _files.move(0, index);
        _files[0]->size = size;
    }
    return writeIndex();
}

bool ldomDocCacheImpl::init() {
    CRLog::info("Initialize document cache in directory %s", UnicodeToUtf8(_cacheDir).c_str());
    // read index
    if (readIndex()) {
        // read successfully
        // remove files not specified in list
        removeExtraFiles();
    } else {
        if (!LVCreateDirectory(_cacheDir)) {
            CRLog::error("Document Cache: cannot create cache directory %s, disabling cache", UnicodeToUtf8(_cacheDir).c_str());
            return false;
        }
        _files.clear();
    }
    reserve(0);
    if (!writeIndex())
        return false; // cannot write index: read only?
    return true;
}

bool ldomDocCacheImpl::clear() {
    for (int i = 0; i < _files.length(); i++)
        LVDeleteFile(_cacheDir + _files[i]->filename);
    _files.clear();
    return writeIndex();
}

lString32 ldomDocCacheImpl::makeFileName(lString32 filename, lUInt32 crc, lUInt32 docFlags) {
    lString32 fn;
    lString8 filename8 = UnicodeToTranslit(filename);
    bool lastUnderscore = false;
    int goodCount = 0;
    int badCount = 0;
    for (int i = 0; i < filename8.length(); i++) {
        lChar32 ch = filename8[i];

        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '.' || ch == '-') {
            fn << ch;
            lastUnderscore = false;
            goodCount++;
        } else {
            if (!lastUnderscore) {
                fn << U"_";
                lastUnderscore = true;
            }
            badCount++;
        }
    }
    if (goodCount < 2 || badCount > goodCount * 2)
        fn << "_noname";
    if (fn.length() > 25)
        fn = fn.substr(0, 12) + "-" + fn.substr(fn.length() - 12, 12);
    char s[16];
    sprintf(s, ".%08x.%d.cr3", (unsigned)crc, (int)docFlags);
    return fn + lString32(s); //_cacheDir +
}

LVStreamRef ldomDocCacheImpl::openExisting(lString32 filename, lUInt32 crc, lUInt32 docFlags, lString32& cachePath) {
    lString32 fn = makeFileName(filename, crc, docFlags);
    CRLog::debug("ldomDocCache::openExisting(%s)", LCSTR(fn));
    // Try filename with ".keep" extension (that a user can manually add
    // to a .cr3 cache file, for it to no more be maintained by crengine
    // in its index, thus not subject to _maxSize enforcement, so sure
    // to not be deleted by crengine)
    lString32 fn_keep = _cacheDir + fn + ".keep";
    if (LVFileExists(fn_keep)) {
        LVStreamRef stream = LVOpenFileStream(fn_keep.c_str(), LVOM_APPEND | LVOM_FLAG_SYNC);
        if (!stream.isNull()) {
            CRLog::info("ldomDocCache::openExisting - opening user renamed cache file %s", UnicodeToUtf8(fn_keep).c_str());
            cachePath = fn_keep;
#if ENABLED_BLOCK_WRITE_CACHE
            stream = LVCreateBlockWriteStream(stream, WRITE_CACHE_BLOCK_SIZE, WRITE_CACHE_BLOCK_COUNT);
#endif
            return stream;
        }
    }
    LVStreamRef res;
    if (findFileIndex(fn) < 0) {
        CRLog::error("ldomDocCache::openExisting - File %s is not found in cache index", UnicodeToUtf8(fn).c_str());
        return res;
    }
    lString32 pathname = _cacheDir + fn;
    res = LVOpenFileStream(pathname.c_str(), LVOM_APPEND | LVOM_FLAG_SYNC);
    if (!res) {
        CRLog::error("ldomDocCache::openExisting - File %s is listed in cache index, but cannot be opened", UnicodeToUtf8(fn).c_str());
        return res;
    }
    cachePath = pathname;

#if ENABLED_BLOCK_WRITE_CACHE
    res = LVCreateBlockWriteStream(res, WRITE_CACHE_BLOCK_SIZE, WRITE_CACHE_BLOCK_COUNT);
#if TEST_BLOCK_STREAM

    LVStreamRef stream2 = LVOpenFileStream((_cacheDir + fn + "_c").c_str(), LVOM_APPEND);
    if (!stream2) {
        CRLog::error("ldomDocCache::openExisting - file %s is cannot be created", UnicodeToUtf8(fn).c_str());
        return stream2;
    }
    res = LVCreateCompareTestStream(res, stream2);
#endif
#endif

    lUInt32 fileSize = (lUInt32)res->GetSize();
    moveFileToTop(fn, fileSize);
    return res;
}

LVStreamRef ldomDocCacheImpl::createNew(lString32 filename, lUInt32 crc, lUInt32 docFlags, lUInt32 fileSize, lString32& cachePath) {
    lString32 fn = makeFileName(filename, crc, docFlags);
    LVStreamRef res;
    lString32 pathname = _cacheDir + fn;
    // If this cache filename exists with a ".keep" extension (manually
    // added by the user), and we were going to create a new one (because
    // this .keep is invalid, or cache file format version has changed),
    // remove it and create the new one with this same .keep extension,
    // so it stays (as wished by the user) not maintained by crengine.
    lString32 fn_keep = pathname + ".keep";
    if (LVFileExists(fn_keep)) {
        LVDeleteFile(pathname); // delete .cr3 if any
        LVDeleteFile(fn_keep);  // delete invalid .cr3.keep
        LVStreamRef stream = LVOpenFileStream(fn_keep.c_str(), LVOM_APPEND | LVOM_FLAG_SYNC);
        if (!stream.isNull()) {
            CRLog::info("ldomDocCache::createNew - re-creating user renamed cache file %s", UnicodeToUtf8(fn_keep).c_str());
            cachePath = fn_keep;
#if ENABLED_BLOCK_WRITE_CACHE
            stream = LVCreateBlockWriteStream(stream, WRITE_CACHE_BLOCK_SIZE, WRITE_CACHE_BLOCK_COUNT);
#endif
            return stream;
        }
    }
    if (findFileIndex(pathname) >= 0)
        LVDeleteFile(pathname);
    reserve(fileSize / 10);
    //res = LVMapFileStream( (_cacheDir+fn).c_str(), LVOM_APPEND, fileSize );
    LVDeleteFile(pathname); // try to delete, ignore errors
    res = LVOpenFileStream(pathname.c_str(), LVOM_APPEND | LVOM_FLAG_SYNC);
    if (!res) {
        CRLog::error("ldomDocCache::createNew - file %s is cannot be created", UnicodeToUtf8(fn).c_str());
        return res;
    }
    cachePath = pathname;
#if ENABLED_BLOCK_WRITE_CACHE
    res = LVCreateBlockWriteStream(res, WRITE_CACHE_BLOCK_SIZE, WRITE_CACHE_BLOCK_COUNT);
#if TEST_BLOCK_STREAM
    LVStreamRef stream2 = LVOpenFileStream((pathname + U"_c").c_str(), LVOM_APPEND);
    if (!stream2) {
        CRLog::error("ldomDocCache::createNew - file %s is cannot be created", UnicodeToUtf8(fn).c_str());
        return stream2;
    }
    res = LVCreateCompareTestStream(res, stream2);
#endif
#endif
    moveFileToTop(fn, fileSize);
    return res;
}
