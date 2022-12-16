/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2011,2012 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2014 Huang Xin <chrox.huang@gmail.com>                  *
 *   Copyright (C) 2017,2018 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
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

#ifndef __LDOMWORDSCOLLECTOR_H_INCLUDED__
#define __LDOMWORDSCOLLECTOR_H_INCLUDED__

#include <ldomnodecallback.h>
#include <ldomword.h>
#include <ldomxrange.h>

class ldomWordsCollector: public ldomNodeCallback
{
    LVArray<ldomWord>& _list;
    ldomWordsCollector& operator=(ldomWordsCollector&) {
        // no assignment
        return *this;
    }
public:
    ldomWordsCollector(LVArray<ldomWord>& list)
            : _list(list) {
    }
    /// called for each found text fragment in range
    virtual void onText(ldomXRange* nodeRange) {
        ldomNode* node = nodeRange->getStart().getNode();
        lString32 text = node->getText();
        int len = text.length();
        int end = nodeRange->getEnd().getOffset();
        if (len > end)
            len = end;
        int beginOfWord = -1;
        for (int i = nodeRange->getStart().getOffset(); i <= len; i++) {
            // int alpha = lGetCharProps(text[i]) & CH_PROP_ALPHA;
            // Also allow digits (years, page numbers) to be considered words
            // int alpha = lGetCharProps(text[i]) & (CH_PROP_ALPHA|CH_PROP_DIGIT|CH_PROP_HYPHEN);
            // We use lStr_isWordSeparator() as the other word finding/skipping functions do,
            // so they all share the same notion of what a word is.
            int alpha = !lStr_isWordSeparator(text[i]); // alpha, number, CJK char
            if (alpha && beginOfWord < 0) {
                beginOfWord = i;
            }
            if (!alpha && beginOfWord >= 0) { // space, punctuation, sign, paren...
                _list.add(ldomWord(node, beginOfWord, i));
                beginOfWord = -1;
            }
            if (lGetCharProps(text[i]) == CH_PROP_CJK && i < len) { // a CJK char makes its own word
                _list.add(ldomWord(node, i, i + 1));
                beginOfWord = -1;
            }
        }
    }
    /// called for each found node in range
    virtual bool onElement(ldomXPointerEx* ptr) {
        ldomNode* elem = ptr->getNode();
        if (elem->getRendMethod() == erm_invisible)
            return false;
        return true;
    }
};

#endif // __LDOMWORDSCOLLECTOR_H_INCLUDED__
