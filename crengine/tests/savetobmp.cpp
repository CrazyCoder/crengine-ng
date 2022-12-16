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
 * \file savetobmp.cpp
 * \brief Save LVDrawBuf into bmp-file.
 * 
 * Only color 16/24/32 bit images are supported!
 */

#include "savetobmp.h"

#include <lvstreamutils.h>
#include <lvbyteorder.h>

#include <string.h>

namespace crengine_ng
{

namespace unittesting
{

bool saveToBMP(const char* filename, LVDrawBufRef drawbuf) {
    if (drawbuf.isNull())
        return false;
    bool res = false;
    lUInt32 width = drawbuf->GetWidth();
    lUInt32 height = drawbuf->GetHeight();
    lUInt32 bpp = drawbuf->GetBitsPerPixel();
    if (bpp < 16)
        return res;
    lUInt32 lineSz = width * (bpp >> 3);
    lUInt32 padding = (4 - (lineSz % 4)) & 0x03;
    lUInt32 pitch = lineSz + padding;

    lUInt32 fileSize = 14 /*FILE_HEADER_SIZE*/ + 40 /*INFO_HEADER_SIZE*/ + (pitch * height);
    LVStreamRef stream = LVOpenFileStream(filename, LVOM_WRITE);
    if (!stream.isNull()) {
        // file header
        *stream << (unsigned char)'B'; // 0
        *stream << (unsigned char)'M'; // 1
        lvByteOrderConv cnv;
        *stream << cnv.lsf(fileSize); // 2
        *stream << (unsigned char)0;  // 6
        *stream << (unsigned char)0;  // 7
        *stream << (unsigned char)0;  // 8
        *stream << (unsigned char)0;  // 9
        *stream << (unsigned char)54; // 10
        *stream << (unsigned char)0;  // 11
        *stream << (unsigned char)0;  // 12
        *stream << (unsigned char)0;  // 13
        // bitmap info header
        *stream << (unsigned char)40;  // 0
        *stream << (unsigned char)0;   // 1
        *stream << (unsigned char)0;   // 2
        *stream << (unsigned char)0;   // 3
        *stream << cnv.lsf(width);     // 4
        *stream << cnv.lsf(height);    // 8
        *stream << (unsigned char)1;   // 12
        *stream << (unsigned char)0;   // 13
        *stream << (unsigned char)bpp; // 14
        *stream << (unsigned char)0;   // 15
        *stream << (unsigned char)0;   // 16
        *stream << (unsigned char)0;   // 17
        *stream << (unsigned char)0;   // 18
        *stream << (unsigned char)0;   // 19
        *stream << (unsigned char)0;   // 20
        *stream << (unsigned char)0;   // 21
        *stream << (unsigned char)0;   // 22
        *stream << (unsigned char)0;   // 23
        *stream << (unsigned char)0;   // 24
        *stream << (unsigned char)0;   // 25
        *stream << (unsigned char)0;   // 26
        *stream << (unsigned char)0;   // 27
        *stream << (unsigned char)0;   // 28
        *stream << (unsigned char)0;   // 29
        *stream << (unsigned char)0;   // 30
        *stream << (unsigned char)0;   // 31
        *stream << (unsigned char)0;   // 32
        *stream << (unsigned char)0;   // 33
        *stream << (unsigned char)0;   // 34
        *stream << (unsigned char)0;   // 35
        *stream << (unsigned char)0;   // 36
        *stream << (unsigned char)0;   // 37
        *stream << (unsigned char)0;   // 38
        *stream << (unsigned char)0;   // 39
        // image data
        lUInt8* data = (lUInt8*)malloc(pitch);
        if (data) {
            memset(data, 0, pitch);
            lvsize_t dw;
            lverror_t ret;
            for (int y = height - 1; y >= 0; y--) {
                const lUInt8* line = drawbuf->GetScanLine(y);
                memcpy(data, line, lineSz);
                ret = stream->Write(data, pitch, &dw);
                if (ret != LVERR_OK || dw != pitch)
                    break;
            }
            free(data);
            res = ret == LVERR_OK;
        }
    }
    return res;
}

} // namespace unittesting

} // namespace crengine_ng
