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

#ifndef __CACHELOADINGCALLBACK_H_INCLUDED__
#define __CACHELOADINGCALLBACK_H_INCLUDED__

#include <crbookformats.h>

class CacheLoadingCallback
{
public:
    /// called when format of document being loaded from cache became known
    virtual void OnCacheFileFormatDetected(doc_format_t) = 0;
    virtual ~CacheLoadingCallback() { }
};

#endif // __CACHELOADINGCALLBACK_H_INCLUDED__
