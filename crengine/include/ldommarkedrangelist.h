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
