/**
 *  crengine-ng
 *  Copyright (c) 2022 Aleksey Chernov
 *
 *  \file calc-drawbuf-diff.h
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

#ifndef _CALC_DRAWBUF_DIFF_H
#define _CALC_DRAWBUF_DIFF_H

#include <lvdrawbuf.h>

class LVDrawBuf;

namespace crengine_ng
{

namespace unittesting
{

/**
 * @brief Calculate difference between two LVDrawBuf instances and return difference as other LVDrawBuf.
 * @param buf1 First drawbuf
 * @param buf2 Second drawbuf
 * @return LVDrawBuf reference as image difference or null reference if buf1 and buf2 have incompatible formats.
 */
LVDrawBufRef calcDrawBufDiff(LVDrawBufRef buf1, LVDrawBufRef buf2);

/**
 * @brief Validate drawbuf, obtained using the calcDrawBufDiff() function, as a mostly black image.
 * @param buf Source drawbuf, allowed only 16/32 color.
 * @param maxColorDiff the value of the maximum allowable deviation of one color component
 * @param maxToleranceCount The maximum allowed number of dots with a color deviation allowed.
 * @param maxErrorsCount The maximum allowed number of points with non-permissible color deviation.
 * @return true if this drawbuf is basically a black image, false otherwise.
 */
bool validateDrawBufDiff(LVDrawBufRef buf, lUInt8 maxColorDiff, lUInt32 maxToleranceCount, lUInt32 maxErrorsCount);

} // namespace unittesting

} // namespace crengine_ng

#endif // _CALC_DRAWBUF_DIFF_H
