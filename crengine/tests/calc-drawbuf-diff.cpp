/**
 *  crengine-ng
 *  Copyright (c) 2022 Aleksey Chernov
 *
 *  \file calc-drawbuf-diff.cpp
 *  \brief crengine-ng unit test module.
 *  Calculate difference between two LVDrawBuf instances and return difference as other LVDrawBuf.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "calc-drawbuf-diff.h"

#include <lvcolordrawbuf.h>
#include <crlog.h>

namespace crengine_ng
{

namespace unittesting
{

static inline lUInt8 _abs_diff(lUInt8 v1, lUInt8 v2) {
    if (v1 > v2)
        return v1 - v2;
    return v2 - v1;
}

LVDrawBufRef calcDrawBufDiff(LVDrawBufRef buf1, LVDrawBufRef buf2) {
    LVDrawBufRef ref;
    if (buf1.isNull() || buf2.isNull())
        return ref;
    if (buf1->GetBitsPerPixel() != buf2->GetBitsPerPixel())
        return ref;
    if (buf1->GetWidth() != buf2->GetWidth())
        return ref;
    if (buf1->GetHeight() != buf2->GetHeight())
        return ref;
    if (buf1->GetWidth() == 0 || buf1->GetHeight() == 0)
        return ref;
    int width = buf1->GetWidth();
    int height = buf1->GetHeight();
    LVDrawBuf* buf = new LVColorDrawBuf(width, height, 32);
    for (int y = 0; y < buf->GetHeight(); y++) {
        lUInt32* dst_line = (lUInt32*)buf->GetScanLine(y);
        for (int x = 0; x < buf->GetWidth(); x++) {
            lUInt32 c1 = buf1->GetPixel(x, y);
            lUInt32 c2 = buf2->GetPixel(x, y);
            lUInt8 c1_1 = c1 >> 24;
            lUInt8 c1_2 = (c1 >> 16) & 0x00FF;
            lUInt8 c1_3 = (c1 >> 8) & 0x00FF;
            lUInt8 c1_4 = c1 & 0x00FF;
            lUInt8 c2_1 = c2 >> 24;
            lUInt8 c2_2 = (c2 >> 16) & 0x00FF;
            lUInt8 c2_3 = (c2 >> 8) & 0x00FF;
            lUInt8 c2_4 = c2 & 0x00FF;
            lUInt8 c1_diff = _abs_diff(c1_1, c2_1);
            lUInt8 c2_diff = _abs_diff(c1_2, c2_2);
            lUInt8 c3_diff = _abs_diff(c1_3, c2_3);
            lUInt8 c4_diff = _abs_diff(c1_4, c2_4);
            dst_line[x] = (c1_diff << 24) | (c2_diff << 16) | (c3_diff << 8) | c4_diff;
        }
    }
    ref = LVDrawBufRef(buf);
    return ref;
}

bool validateDrawBuf(LVDrawBufRef buf, lUInt8 maxColorDiff, lUInt32 maxToleranceCount, lUInt32 maxErrorsCount) {
    if (buf.isNull())
        return false;
    if (buf->GetWidth() == 0 || buf->GetHeight() == 0)
        return false;
    if (buf->GetBitsPerPixel() != 32 && buf->GetBitsPerPixel() != 16)
        return false;
    bool valid = true;
    lUInt32 toleranceCount = 0;
    lUInt32 errorsCount = 0;
    lUInt32 maxToleranceVal = 0;
    for (int y = 0; y < buf->GetHeight(); y++) {
        for (int x = 0; x < buf->GetWidth(); x++) {
            lUInt32 c = buf->GetPixel(x, y);
            lUInt8 c_1 = c >> 24;
            lUInt8 c_2 = (c >> 16) & 0x00FF;
            lUInt8 c_3 = (c >> 8) & 0x00FF;
            lUInt8 c_4 = c & 0x00FF;
            if (c_1 > maxColorDiff)
                errorsCount++;
            else if (c_1 > 0) {
                toleranceCount++;
                if (maxToleranceVal < c_1)
                    maxToleranceVal = c_1;
            }
            if (c_2 > maxColorDiff)
                errorsCount++;
            else if (c_2 > 0) {
                toleranceCount++;
                if (maxToleranceVal < c_2)
                    maxToleranceVal = c_2;
            }
            if (c_3 > maxColorDiff)
                errorsCount++;
            else if (c_3 > 0) {
                toleranceCount++;
                if (maxToleranceVal < c_3)
                    maxToleranceVal = c_3;
            }
            if (c_4 > maxColorDiff)
                errorsCount++;
            else if (c_4 > 0) {
                toleranceCount++;
                if (maxToleranceVal < c_4)
                    maxToleranceVal = c_4;
            }
        }
    }
    if (errorsCount > maxErrorsCount)
        valid = false;
    if (toleranceCount > maxToleranceCount)
        valid = false;
    CRLog::trace("validateDrawBuf(): errorsCount=%u", errorsCount);
    CRLog::trace("validateDrawBuf(): maxToleranceVal=%u", maxToleranceVal);
    CRLog::trace("validateDrawBuf(): totalToleranceCount=%u", toleranceCount);
    return valid;
}

} // namespace unittesting

} // namespace crengine_ng
