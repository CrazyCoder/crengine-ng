/***************************************************************************
 *   crengine-ng, unit testing                                             *
 *   Copyright (C) 2022 Aleksey Chernov <valexlin@gmail.com>               *
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

/**
 * \file compare-two-textfiles.cpp
 * \brief Comparison of two text files.
 * 
 * One or both files may be a gzip archive.
 * In this case, the contents of the archive are compared.
 */

#include "compare-two-textfiles.h"

#include <crsetup.h>

#if USE_ZLIB != 1
#error This module does not work without zlib!
#endif

#include <zlib.h>

#include <string.h> // for memcmp()

namespace crengine_ng
{

namespace unittesting
{

static inline bool my_isspace(int c) {
    return (c == 0x20) || (c == 0x07) || (c == 0x0A) || (c == 0x0D);
}

#define READ_BUFF_SZ 4096

bool compareTwoTextFiles(const char* file1, const char* file2) {
    gzFile gzf1 = gzopen(file1, "rt");
    if (gzf1 == NULL)
        return false;
    gzFile gzf2 = gzopen(file2, "rt");
    if (gzf2 == NULL) {
        gzclose(gzf1);
        return false;
    }
    int eof1, eof2;
    unsigned int dw1, dw2;
    unsigned char buf1[READ_BUFF_SZ];
    unsigned char buf2[READ_BUFF_SZ];
    unsigned char swap_buf[READ_BUFF_SZ];
    bool diff = false;
    bool error = false;
    unsigned char* read1_ptr = &buf1[0];
    unsigned char* read2_ptr = &buf2[0];
    unsigned int read1_sz = READ_BUFF_SZ;
    unsigned int read2_sz = READ_BUFF_SZ;
    unsigned int buf1_unread_sz = 0;
    unsigned int buf2_unread_sz = 0;
    for (;;) {
        // read parts
        eof1 = gzeof(gzf1);
        eof2 = gzeof(gzf2);
        if (eof1 != eof2) {
            diff = true;
            break;
        }
        if (eof1 || eof2)
            break;
        int ret1 = gzread(gzf1, read1_ptr, read1_sz);
        int ret2 = gzread(gzf2, read2_ptr, read2_sz);
        if (ret1 < 0 || ret2 < 0) {
            error = true;
            break;
        }
        dw1 = (unsigned int)ret1 + buf1_unread_sz;
        dw2 = (unsigned int)ret2 + buf2_unread_sz;
        // compare parts
        unsigned char* cmp1_ptr = &buf1[0];
        unsigned char* cmp2_ptr = &buf2[0];
        unsigned int buf1_pos = 0;
        unsigned int buf2_pos = 0;
        bool buf1_ready = false;
        bool buf2_ready = false;
        bool buf1_space_seek_ovf = false;
        bool buf2_space_seek_ovf = false;
        while (buf1_pos < dw1 && buf2_pos < dw2) {
            if (my_isspace(*cmp1_ptr)) {
                cmp1_ptr++;
                buf1_pos++;
                while (buf1_pos < dw1 && my_isspace(*cmp1_ptr)) {
                    cmp1_ptr++;
                    buf1_pos++;
                }
                if (buf1_pos == dw1)
                    buf1_space_seek_ovf = true;
                cmp1_ptr--;
                buf1_pos--;
                *cmp1_ptr = 0x20;
            }
            if (my_isspace(*cmp2_ptr)) {
                cmp2_ptr++;
                buf2_pos++;
                while (buf2_pos < dw2 && my_isspace(*cmp2_ptr)) {
                    cmp2_ptr++;
                    buf2_pos++;
                }
                if (buf2_pos == dw2)
                    buf2_space_seek_ovf = true;
                cmp2_ptr--;
                buf2_pos--;
                *cmp2_ptr = 0x20;
            }
            if (buf1_space_seek_ovf) {
                buf2_unread_sz = dw2 - buf2_pos;
                if (buf2_unread_sz > 0) {
                    memcpy(swap_buf, buf2 + buf2_pos, buf2_unread_sz);
                    memcpy(buf2, swap_buf, buf2_unread_sz);
                }
                read2_ptr = buf2 + buf2_unread_sz;
                read2_sz = READ_BUFF_SZ - buf2_unread_sz;
                buf2_ready = true;
                break;
            }
            if (buf2_space_seek_ovf) {
                buf1_unread_sz = dw1 - buf1_pos;
                if (buf1_unread_sz > 0) {
                    memcpy(swap_buf, buf1 + buf1_pos, buf1_unread_sz);
                    memcpy(buf1, swap_buf, buf1_unread_sz);
                }
                read1_ptr = buf1 + buf1_unread_sz;
                read1_sz = READ_BUFF_SZ - buf1_unread_sz;
                buf1_ready = true;
                break;
            }
            diff = *cmp1_ptr != *cmp2_ptr;
            if (diff)
                break;
            cmp1_ptr++;
            buf1_pos++;
            cmp2_ptr++;
            buf2_pos++;
        }
        if (diff)
            break;
        if (!buf1_ready) {
            buf1_unread_sz = dw1 - buf1_pos;
            if (buf1_unread_sz > 0) {
                memcpy(swap_buf, buf1 + buf1_pos, buf1_unread_sz);
                memcpy(buf1, swap_buf, buf1_unread_sz);
            }
            read1_ptr = buf1 + buf1_unread_sz;
            read1_sz = READ_BUFF_SZ - buf1_unread_sz;
        }
        if (!buf2_ready) {
            buf2_unread_sz = dw2 - buf2_pos;
            if (buf2_unread_sz > 0) {
                memcpy(swap_buf, buf2 + buf2_pos, buf2_unread_sz);
                memcpy(buf2, swap_buf, buf2_unread_sz);
            }
            read2_ptr = buf2 + buf2_unread_sz;
            read2_sz = READ_BUFF_SZ - buf2_unread_sz;
        }
    }
    gzclose(gzf1);
    gzclose(gzf2);
    return !error && !diff;
}

} // namespace unittesting

} // namespace crengine_ng
