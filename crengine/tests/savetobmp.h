/**
 *  crengine-ng
 *  Copyright (c) 2022 Aleksey Chernov
 *
 *  \file savetobmp.h
 *  \brief crengine-ng unit test module.
 *  Save LVDrawBuf into bmp-file.
 *  Only color 16/24/32 bit images are supported!
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SAVETOBMP_H
#define _SAVETOBMP_H

#include <lvdrawbuf.h>

namespace crengine_ng
{

namespace unittesting
{

bool saveToBMP(const char* filename, LVDrawBufRef drawbuf);

} // namespace unittesting

} // namespace crengine_ng

#endif // _SAVETOBMP_H
