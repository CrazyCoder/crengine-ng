/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2021,2024 Aleksey Chernov <valexlin@gmail.com>          *
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

#ifndef __LVSTREAM_LSEEK_H_INCLUDED__
#define __LVSTREAM_LSEEK_H_INCLUDED__

#include <lvtypes.h>

#if !defined(__SYMBIAN32__) && defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

// To support "large files" on 32-bit platforms
// Since we have defined own types 'lvoffset_t', 'lvpos_t' and do not use the system type 'off_t'
// it is logical to define our own wrapper function 'lseek'.
static inline lvpos_t cr3_lseek(int fd, lvoffset_t offset, int whence) {
#if LVLONG_FILE_SUPPORT == 1 && (!defined(MACOS) || MACOS == 0) && !defined(__FreeBSD__)
    return (lvpos_t)::lseek64(fd, (off64_t)offset, whence);
#else
    return (lvpos_t)::lseek(fd, (off_t)offset, whence);
#endif
}

#endif // __LVSTREAM_LSEEK_H_INCLUDED__
