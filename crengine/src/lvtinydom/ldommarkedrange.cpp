/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007,2011,2012 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2011 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2019,2020 poire-z <poire-z@users.noreply.github.com>    *
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

#include "ldommarkedrange.h"

/// returns true if intersects specified line rectangle
bool ldomMarkedRange::intersects(lvRect& rc, lvRect& intersection) {
    if (flags < 0x10) {
        // This assumes lines (rc) are from full-width LTR paragraphs, and
        // takes some shortcuts when checking intersection (it can be wrong
        // when floats, table cells, or RTL/BiDi text are involved).
        if (start.y >= rc.bottom)
            return false;
        if (end.y < rc.top)
            return false;
        intersection = rc;
        if (start.y >= rc.top && start.y < rc.bottom) {
            if (start.x > rc.right)
                return false;
            intersection.left = rc.left > start.x ? rc.left : start.x;
        }
        if (end.y >= rc.top && end.y < rc.bottom) {
            if (end.x < rc.left)
                return false;
            intersection.right = rc.right < end.x ? rc.right : end.x;
        }
        return true;
    } else {
        // Don't take any shortcut and check the full intersection
        if (rc.bottom <= start.y || rc.top >= end.y || rc.right <= start.x || rc.left >= end.x) {
            return false; // no intersection
        }
        intersection.top = rc.top > start.y ? rc.top : start.y;
        intersection.bottom = rc.bottom < end.y ? rc.bottom : end.y;
        intersection.left = rc.left > start.x ? rc.left : start.x;
        intersection.right = rc.right < end.x ? rc.right : end.x;
        return !intersection.isEmpty();
    }
}

lvPoint ldomMarkedRange::getMiddlePoint() {
    if (start.y == end.y) {
        return lvPoint(((start.x + end.x) >> 1), start.y);
    } else {
        return start;
    }
}

/// returns distance (dx+dy) from specified point to middle point
int ldomMarkedRange::calcDistance(int x, int y, MoveDirection dir) {
    lvPoint middle = getMiddlePoint();
    int dx = middle.x - x;
    int dy = middle.y - y;
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    switch (dir) {
        case DIR_LEFT:
        case DIR_RIGHT:
            return dx + dy;
        case DIR_UP:
        case DIR_DOWN:
            return dx + dy * 100;
        case DIR_ANY:
            return dx + dy;
    }

    return dx + dy;
}

// class ldomMarkedRangeList

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
