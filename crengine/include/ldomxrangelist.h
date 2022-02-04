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

#ifndef __LDOMXRANGELIST_H_INCLUDED__
#define __LDOMXRANGELIST_H_INCLUDED__

#include <ldomxrange.h>

class ldomMarkedText
{
public:
    lString32 text;
    lUInt32 flags;
    int offset;
    ldomMarkedText(lString32 s, lUInt32 flg, int offs)
            : text(s)
            , flags(flg)
            , offset(offs) {
    }
    ldomMarkedText(const ldomMarkedText& v)
            : text(v.text)
            , flags(v.flags) {
    }
};

typedef LVPtrVector<ldomMarkedText> ldomMarkedTextList;

class ldomXRangeList: public LVPtrVector<ldomXRange>
{
public:
    /// add ranges for words
    void addWords(const LVArray<ldomWord>& words) {
        for (int i = 0; i < words.length(); i++)
            LVPtrVector<ldomXRange>::add(new ldomXRange(words[i]));
    }
    ldomXRangeList(const LVArray<ldomWord>& words) {
        addWords(words);
    }
    /// create list splittiny existing list into non-overlapping ranges
    ldomXRangeList(ldomXRangeList& srcList, bool splitIntersections);
    /// create list by filtering existing list, to get only values which intersect filter range
    ldomXRangeList(ldomXRangeList& srcList, ldomXRange& filter);
    /// fill text selection list by splitting text into monotonic flags ranges
    void splitText(ldomMarkedTextList& dst, ldomNode* textNodeToSplit);
    /// fill marked ranges list
    void getRanges(ldomMarkedRangeList& dst);
    /// split into subranges using intersection
    void split(ldomXRange* r);
    /// default constructor for empty list
    ldomXRangeList() {};
};

#endif // __LDOMXRANGELIST_H_INCLUDED__
