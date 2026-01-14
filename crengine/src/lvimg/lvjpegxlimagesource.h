/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2026 Aleksey Chernov <valexlin@gmail.com>               *
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

#ifndef LVJPEGXLIMAGESOURCE_H_INCLUDED
#define LVJPEGXLIMAGESOURCE_H_INCLUDED

#include <crsetup.h>

#if (USE_LIBJXL == 1)

#include "lvnodeimagesource.h"

class LVJpegXLImageSource: public LVNodeImageSource
{
public:
    LVJpegXLImageSource(ldomNode* node, LVStreamRef stream);
    virtual ~LVJpegXLImageSource();
    virtual void Compact();
    virtual bool Decode(LVImageDecoderCallback* callback);
    static bool CheckPattern(const lUInt8* buf, int len);
};

#endif // (USE_LIBJXL == 1)

#endif // LVJPEGXLIMAGESOURCE_H_INCLUDED
