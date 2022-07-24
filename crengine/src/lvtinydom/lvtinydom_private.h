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

#ifndef __LV_TINYDOM_PRIVATE_H_INCLUDED__
#define __LV_TINYDOM_PRIVATE_H_INCLUDED__

/// change in case of incompatible changes in swap/cache file format to avoid using incompatible swap file
#define CACHE_FILE_FORMAT_VERSION "3.12.80"

/// increment following value to force re-formatting of old book after load
#define FORMATTING_VERSION_ID 0x0028

#define COMPRESS_NODE_DATA         true
#define COMPRESS_NODE_STORAGE_DATA true
#define COMPRESS_MISC_DATA         true
#define COMPRESS_PAGES_DATA        true
#define COMPRESS_TOC_DATA          true
#define COMPRESS_PAGEMAP_DATA      true
#define COMPRESS_STYLE_DATA        true

/// set t 1 to log storage reads/writes
#define DEBUG_DOM_STORAGE 0

#ifndef DOC_BUFFER_SIZE
#define DOC_BUFFER_SIZE 0x00A00000UL // default buffer size
#endif

#if DOC_BUFFER_SIZE >= 0x7FFFFFFFUL
#error DOC_BUFFER_SIZE value is too large. This results in integer overflow.
#endif

#endif // __LV_TINYDOM_PRIVATE_H_INCLUDED__
