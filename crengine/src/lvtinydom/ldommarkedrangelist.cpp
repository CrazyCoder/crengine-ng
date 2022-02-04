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
