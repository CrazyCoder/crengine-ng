/**
 * CoolReader Engine
 *
 * (c) Vadim Lopatin, 2000-2011
 * (c) Other CoolReader authors (See AUTHORS file)
 *
 * This source code is distributed under the terms of
 * GNU General Public License version 2
 * See LICENSE file for details
 */

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
