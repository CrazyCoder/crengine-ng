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

#include <ldomxrangelist.h>
#include <ldommarkedrange.h>
#include <ldommarkedrangelist.h>

/// create list by filtering existing list, to get only values which intersect filter range
ldomXRangeList::ldomXRangeList(ldomXRangeList& srcList, ldomXRange& filter) {
    for (int i = 0; i < srcList.length(); i++) {
        if (srcList[i]->checkIntersection(filter))
            LVPtrVector<ldomXRange>::add(new ldomXRange(*srcList[i]));
    }
}

/// create list splittiny existing list into non-overlapping ranges
ldomXRangeList::ldomXRangeList(ldomXRangeList& srcList, bool splitIntersections) {
    if (srcList.empty())
        return;
    int i;
    if (splitIntersections) {
        ldomXRange* maxRange = new ldomXRange(*srcList[0]);
        for (i = 1; i < srcList.length(); i++) {
            if (srcList[i]->getStart().compare(maxRange->getStart()) < 0)
                maxRange->setStart(srcList[i]->getStart());
            if (srcList[i]->getEnd().compare(maxRange->getEnd()) > 0)
                maxRange->setEnd(srcList[i]->getEnd());
        }
        maxRange->setFlags(0);
        add(maxRange);
        for (i = 0; i < srcList.length(); i++)
            split(srcList[i]);
        for (int i = length() - 1; i >= 0; i--) {
            if (get(i)->getFlags() == 0)
                erase(i, 1);
        }
    } else {
        for (i = 0; i < srcList.length(); i++)
            add(new ldomXRange(*srcList[i]));
    }
}

/// split into subranges using intersection
void ldomXRangeList::split(ldomXRange* r) {
    int i;
    for (i = 0; i < length(); i++) {
        if (r->checkIntersection(*get(i))) {
            ldomXRange* src = remove(i);
            int cmp1 = src->getStart().compare(r->getStart());
            int cmp2 = src->getEnd().compare(r->getEnd());
            //TODO: add intersections
            if (cmp1 < 0 && cmp2 < 0) {
                //   0====== src ======0
                //        X======= r=========X
                //   1111122222222222222
                ldomXRange* r1 = new ldomXRange(src->getStart(), r->getStart(), src->getFlags());
                ldomXRange* r2 = new ldomXRange(r->getStart(), src->getEnd(), src->getFlags() | r->getFlags());
                insert(i++, r1);
                insert(i, r2);
                delete src;
            } else if (cmp1 > 0 && cmp2 > 0) {
                //           0====== src ======0
                //     X======= r=========X
                //           2222222222222233333
                ldomXRange* r2 = new ldomXRange(src->getStart(), r->getEnd(), src->getFlags() | r->getFlags());
                ldomXRange* r3 = new ldomXRange(r->getEnd(), src->getEnd(), src->getFlags());
                insert(i++, r2);
                insert(i, r3);
                delete src;
            } else if (cmp1 < 0 && cmp2 > 0) {
                // 0====== src ================0
                //     X======= r=========X
                ldomXRange* r1 = new ldomXRange(src->getStart(), r->getStart(), src->getFlags());
                ldomXRange* r2 = new ldomXRange(r->getStart(), r->getEnd(), src->getFlags() | r->getFlags());
                ldomXRange* r3 = new ldomXRange(r->getEnd(), src->getEnd(), src->getFlags());
                insert(i++, r1);
                insert(i++, r2);
                insert(i, r3);
                delete src;
            } else if (cmp1 == 0 && cmp2 > 0) {
                //   0====== src ========0
                //   X====== r=====X
                ldomXRange* r1 = new ldomXRange(src->getStart(), r->getEnd(), src->getFlags() | r->getFlags());
                ldomXRange* r2 = new ldomXRange(r->getEnd(), src->getEnd(), src->getFlags());
                insert(i++, r1);
                insert(i, r2);
                delete src;
            } else if (cmp1 < 0 && cmp2 == 0) {
                //   0====== src =====0
                //      X====== r=====X
                ldomXRange* r1 = new ldomXRange(src->getStart(), r->getStart(), src->getFlags());
                ldomXRange* r2 = new ldomXRange(r->getStart(), r->getEnd(), src->getFlags() | r->getFlags());
                insert(i++, r1);
                insert(i, r2);
                delete src;
            } else {
                //        0====== src =====0
                //   X============== r===========X
                //
                //        0====== src =====0
                //   X============== r=====X
                //
                //   0====== src =====0
                //   X============== r=====X
                //
                //   0====== src ========0
                //   X========== r=======X
                src->setFlags(src->getFlags() | r->getFlags());
                insert(i, src);
            }
        }
    }
}

/// fill marked ranges list
// Transform a list of ldomXRange (start and end xpointers) into a list
// of ldomMarkedRange (start and end point coordinates) for native
// drawing of highlights
void ldomXRangeList::getRanges(ldomMarkedRangeList& dst) {
    dst.clear();
    if (empty())
        return;
    for (int i = 0; i < length(); i++) {
        ldomXRange* range = get(i);
        if (range->getFlags() < 0x10) {
            // Legacy marks drawing: make a single ldomMarkedRange spanning
            // multiple lines, assuming full width LTR paragraphs)
            // (Updated to use toPoint(extended=true) to have them shifted
            // by the margins and paddings of final blocks, to be compatible
            // with getSegmentRects() below that does that internally.)
            lvPoint ptStart = range->getStart().toPoint(true);
            lvPoint ptEnd = range->getEnd().toPoint(true);
            // LVE:DEBUG
            // CRLog::trace("selectRange( %d,%d : %d,%d : %s, %s )", ptStart.x, ptStart.y, ptEnd.x, ptEnd.y,
            //              LCSTR(range->getStart().toString()), LCSTR(range->getEnd().toString()) );
            if (ptStart.y > ptEnd.y || (ptStart.y == ptEnd.y && ptStart.x >= ptEnd.x)) {
                // Swap ptStart and ptEnd if coordinates seems inverted (or we would
                // get item->empty()), which is needed for bidi/rtl.
                // Hoping this has no side effect.
                lvPoint ptTmp = ptStart;
                ptStart = ptEnd;
                ptEnd = ptTmp;
            }
            ldomMarkedRange* item = new ldomMarkedRange(ptStart, ptEnd, range->getFlags());
            if (!item->empty())
                dst.add(item);
            else
                delete item;
        } else {
            // Enhanced marks drawing: from a single ldomXRange, make multiple segmented
            // ldomMarkedRange, each spanning a single line.
            LVArray<lvRect> rects;
            range->getSegmentRects(rects);
            for (int i = 0; i < rects.length(); i++) {
                lvRect r = rects[i];
                // printf("r %d %dx%d %dx%d\n", i, r.topLeft().x, r.topLeft().y, r.bottomRight().x, r.bottomRight().y);
                ldomMarkedRange* item = new ldomMarkedRange(r.topLeft(), r.bottomRight(), range->getFlags());
                if (!item->empty())
                    dst.add(item);
                else
                    delete item;
            }
        }
    }
}

/// fill text selection list by splitting text into monotonic flags ranges
void ldomXRangeList::splitText(ldomMarkedTextList& dst, ldomNode* textNodeToSplit) {
    lString32 text = textNodeToSplit->getText();
    if (length() == 0) {
        dst.add(new ldomMarkedText(text, 0, 0));
        return;
    }
    ldomXRange textRange(textNodeToSplit);
    ldomXRangeList ranges;
    ranges.add(new ldomXRange(textRange));
    int i;
    for (i = 0; i < length(); i++) {
        ranges.split(get(i));
    }
    for (i = 0; i < ranges.length(); i++) {
        ldomXRange* r = ranges[i];
        int start = r->getStart().getOffset();
        int end = r->getEnd().getOffset();
        if (end > start)
            dst.add(new ldomMarkedText(text.substr(start, end - start), r->getFlags(), start));
    }
    /*
    if ( dst.length() ) {
        CRLog::debug(" splitted: ");
        for ( int k=0; k<dst.length(); k++ ) {
            CRLog::debug("    (%d, %d) %s", dst[k]->offset, dst[k]->flags, UnicodeToUtf8(dst[k]->text).c_str());
        }
    }
    */
}
