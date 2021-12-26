/*******************************************************

   CoolReader Engine

   lvstream_lseek.h

   (c) Vadim Lopatin, 2000-2009
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#ifndef __LVSTREAM_LSEEK_H_INCLUDED__
#define __LVSTREAM_LSEEK_H_INCLUDED__

#include <lvstream_types.h>

#if !defined(__SYMBIAN32__) && defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

// To support "large files" on 32-bit platforms
// Since we have defined own types 'lvoffset_t', 'lvpos_t' and do not use the system type 'off_t'
// it is logical to define our own wrapper function 'lseek'.
static inline lvpos_t cr3_lseek(int fd, lvoffset_t offset, int whence) {
#if LVLONG_FILE_SUPPORT == 1 && (!defined(MACOS) || MACOS == 0)
    return (lvpos_t)::lseek64(fd, (off64_t)offset, whence);
#else
    return (lvpos_t)::lseek(fd, (off_t)offset, whence);
#endif
}

#endif // __LVSTREAM_LSEEK_H_INCLUDED__
