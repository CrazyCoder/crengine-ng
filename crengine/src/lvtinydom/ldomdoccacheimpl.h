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

#ifndef __LDOMDOCCACHEIMPL_H_INCLUDED__
#define __LDOMDOCCACHEIMPL_H_INCLUDED__

#include <ldomdoccache.h>
#include <lvptrvec.h>

/// document cache
class ldomDocCacheImpl: public ldomDocCache
{
    lString32 _cacheDir;
    lvsize_t _maxSize;
    lUInt32 _oldStreamSize;
    lUInt32 _oldStreamCRC;

    struct FileItem
    {
        lString32 filename;
        lUInt32 size;
    };
    LVPtrVector<FileItem> _files;
public:
    ldomDocCacheImpl(lString32 cacheDir, lvsize_t maxSize);

    bool writeIndex();

    bool readIndex();

    /// remove all .cr3 files which are not listed in index
    bool removeExtraFiles();

    // remove all extra files to add new one of specified size
    bool reserve(lvsize_t allocSize);

    int findFileIndex(lString32 filename);

    bool moveFileToTop(lString32 filename, lUInt32 size);

    bool init();

    /// remove all files
    bool clear();

    // dir/filename.{crc32}.cr3
    lString32 makeFileName(lString32 filename, lUInt32 crc, lUInt32 docFlags);

    /// open existing cache file stream
    LVStreamRef openExisting(lString32 filename, lUInt32 crc, lUInt32 docFlags, lString32& cachePath);

    /// create new cache file
    LVStreamRef createNew(lString32 filename, lUInt32 crc, lUInt32 docFlags, lUInt32 fileSize, lString32& cachePath);

    virtual ~ldomDocCacheImpl() {
    }
};

#endif // __LDOMDOCCACHEIMPL_H_INCLUDED__
