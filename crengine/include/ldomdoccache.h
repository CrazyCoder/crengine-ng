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

#ifndef __LDOMDOCCACHE_H_INCLUDED__
#define __LDOMDOCCACHE_H_INCLUDED__

#include <lvstream.h>

/// document cache
class ldomDocCache
{
public:
    /// open existing cache file stream
    static LVStreamRef openExisting(lString32 filename, lUInt32 crc, lUInt32 docFlags, lString32& cachePath);
    /// create new cache file
    static LVStreamRef createNew(lString32 filename, lUInt32 crc, lUInt32 docFlags, lUInt32 fileSize, lString32& cachePath);
    /// init document cache
    static bool init(lString32 cacheDir, lvsize_t maxSize);
    /// close document cache manager
    static bool close();
    /// delete all cache files
    static bool clear();
    /// returns true if cache is enabled (successfully initialized)
    static bool enabled();
};

#endif // __LDOMDOCCACHE_H_INCLUDED__
