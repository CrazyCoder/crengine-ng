/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2010-2015,2018 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2018-2021 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2020-2023 Aleksey Chernov <valexlin@gmail.com>          *
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

#ifndef __LV_TINYDOM_PRIVATE_H_INCLUDED__
#define __LV_TINYDOM_PRIVATE_H_INCLUDED__

/// change in case of incompatible changes in swap/cache file format to avoid using incompatible swap file
#define CACHE_FILE_FORMAT_VERSION "3.12.81"

/// increment following value to force re-formatting of old book after load
#define FORMATTING_VERSION_ID 0x0030

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
