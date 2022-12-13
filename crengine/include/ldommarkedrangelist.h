/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2019 poire-z <poire-z@users.noreply.github.com>         *
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

#ifndef __LDOMMARKEDRANGELIST_H_INCLUDED__
#define __LDOMMARKEDRANGELIST_H_INCLUDED__

#include <ldommarkedrange.h>

/// list of marked ranges
class ldomMarkedRangeList: public LVPtrVector<ldomMarkedRange>
{
public:
    ldomMarkedRangeList() {
    }
    /// create bounded by RC list, with (0,0) coordinates at left top corner
    // crop/discard elements outside of rc (or outside of crop_rc instead if provided)
    ldomMarkedRangeList(const ldomMarkedRangeList* list, lvRect& rc, lvRect* crop_rc = NULL);
};

#endif // __LDOMMARKEDRANGELIST_H_INCLUDED__
