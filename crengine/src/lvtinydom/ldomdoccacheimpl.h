/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2009,2012 Vadim Lopatin <coolreader.org@gmail.com>      *
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
