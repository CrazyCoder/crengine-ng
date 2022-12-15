/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2011 Konstantin Potapov <pkbo@users.sourceforge.net>    *
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

#include "ldommarkedrangelist.h"

/// create bounded by RC list, with (0,0) coordinates at left top corner
// crop/discard elements outside of rc (or outside of crop_rc instead if provided)
ldomMarkedRangeList::ldomMarkedRangeList(const ldomMarkedRangeList* list, lvRect& rc, lvRect* crop_rc) {
    if (!list || list->empty())
        return;
    //    if ( list->get(0)->start.y>rc.bottom )
    //        return;
    //    if ( list->get( list->length()-1 )->end.y < rc.top )
    //        return;
    if (!crop_rc) {
        // If no alternate crop_rc provided, crop to the rc anchor
        crop_rc = &rc;
    }
    for (int i = 0; i < list->length(); i++) {
        ldomMarkedRange* src = list->get(i);
        if (src->start.y >= crop_rc->bottom || src->end.y < crop_rc->top)
            continue;
        add(new ldomMarkedRange(
                lvPoint(src->start.x - rc.left, src->start.y - rc.top),
                lvPoint(src->end.x - rc.left, src->end.y - rc.top),
                src->flags));
    }
}
