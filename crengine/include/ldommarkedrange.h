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

#ifndef __LDOMMARKEDRANGE_H_INCLUDED__
#define __LDOMMARKEDRANGE_H_INCLUDED__

#include <ldomxpointer.h>
#include <ldomword.h>

enum MoveDirection
{
    DIR_ANY,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN
};

/// range in document, marked with specified flags
class ldomMarkedRange
{
public:
    /// start document point
    lvPoint start;
    /// end document point
    lvPoint end;
    /// flags:
    //  0: not shown
    //  1,2,3: legacy drawing (a single mark may spans multiple lines, assuming full width
    //         LTR paragraphs) (2 & 3 might be used for crengine internal bookmarks,
    //         see hist.h for enum bmk_type)
    //  0x11, 0x12, 0x13:  enhanced drawing (segmented mark, spanning a single line)
    lUInt32 flags;
    bool empty() {
        return (start.y > end.y || (start.y == end.y && start.x >= end.x));
    }
    /// returns mark middle point for single line mark, or start point for multiline mark
    lvPoint getMiddlePoint();
    /// returns distance (dx+dy) from specified point to middle point
    int calcDistance(int x, int y, MoveDirection dir);
    /// returns true if intersects specified line rectangle
    bool intersects(lvRect& rc, lvRect& intersection);
    /// constructor
    ldomMarkedRange(lvPoint _start, lvPoint _end, lUInt32 _flags)
            : start(_start)
            , end(_end)
            , flags(_flags) {
    }
    /// constructor
    ldomMarkedRange(ldomWord& word) {
        ldomXPointer startPos(word.getNode(), word.getStart());
        ldomXPointer endPos(word.getNode(), word.getEnd());
        start = startPos.toPoint();
        end = endPos.toPoint();
    }
    /// copy constructor
    ldomMarkedRange(const ldomMarkedRange& v)
            : start(v.start)
            , end(v.end)
            , flags(v.flags) {
    }
};

#endif // __LDOMMARKEDRANGE_H_INCLUDED__
