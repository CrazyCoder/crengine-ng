/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2010 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2019 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2021 Aleksey Chernov <valexlin@gmail.com>               *
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

#ifndef __CACHEFILEITEM_H_INCLUDED__
#define __CACHEFILEITEM_H_INCLUDED__

#include <lvtypes.h>
#include <crlog.h>

//#define CACHE_FILE_SECTOR_SIZE 4096
#define CACHE_FILE_SECTOR_SIZE 1024

#define CACHE_FILE_ITEM_MAGIC 0xC007B00C

enum CacheCompressionType
{
    CacheCompressionNone = 0,
    CacheCompressionZlib = 1,
    CacheCompressionZSTD = 2,
};

struct CacheFileItem
{
    lUInt32 _magic;            // magic number
    lUInt16 _dataType;         // data type
    lUInt16 _dataIndex;        // additional data index, for internal usage for data type
    int _blockIndex;           // sequential number of block
    int _blockFilePos;         // start of block
    int _blockSize;            // size of block within file
    int _dataSize;             // used data size inside block (<= block size)
    lUInt64 _dataHash;         // additional hash of data
    lUInt64 _packedHash;       // additional hash of packed data
    lUInt32 _uncompressedSize; // size of uncompressed block, if compression is applied, 0 if no compression
    lUInt32 _padding;          // explicite padding (this struct would be implicitely padded from 44 bytes to 48 bytes)
                               // so we can set this padding value to 0 (instead of some random data with implicite padding)
                               // in order to get reproducible (same file checksum) cache files when this gets serialized
    bool validate(int fsize) {
        if (_magic != CACHE_FILE_ITEM_MAGIC) {
            CRLog::error("CacheFileItem::validate: block magic doesn't match");
            return false;
        }
        if (_dataSize > _blockSize || _blockSize < 0 || _dataSize < 0 || _blockFilePos + _dataSize > fsize || _blockFilePos < CACHE_FILE_SECTOR_SIZE) {
            CRLog::error("CacheFileItem::validate: invalid block size or position");
            return false;
        }
        return true;
    }
    CacheFileItem() {
    }
    CacheFileItem(lUInt16 dataType, lUInt16 dataIndex)
            : _magic(CACHE_FILE_ITEM_MAGIC)
            , _dataType(dataType)   // data type
            , _dataIndex(dataIndex) // additional data index, for internal usage for data type
            , _blockIndex(0)        // sequential number of block
            , _blockFilePos(0)      // start of block
            , _blockSize(0)         // size of block within file
            , _dataSize(0)          // used data size inside block (<= block size)
            , _dataHash(0)          // hash of data
            , _packedHash(0)        // additional hash of packed data
            , _uncompressedSize(0)  // size of uncompressed block, if compression is applied, 0 if no compression
            , _padding(0)           // (padding)
    {
    }
};

#endif // __CACHEFILEITEM_H_INCLUDED__
