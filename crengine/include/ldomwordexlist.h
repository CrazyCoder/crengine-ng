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

#ifndef __LDOMWORDEXLIST_H_INCLUDED__
#define __LDOMWORDEXLIST_H_INCLUDED__

#include <ldomwordex.h>

/// list of extended words
class ldomWordExList: public LVPtrVector<ldomWordEx>
{
    int minx;
    int maxx;
    int miny;
    int maxy;
    int x;
    int y;
    ldomWordEx* selWord;
    lString32Collection pattern;
    void init();
    ldomWordEx* findWordByPattern();
public:
    ldomWordExList()
            : minx(-1)
            , maxx(-1)
            , miny(-1)
            , maxy(-1)
            , x(-1)
            , y(-1)
            , selWord(NULL) {
    }
    /// adds all visible words from range, returns number of added words
    int addRangeWords(ldomXRange& range, bool trimPunctuation);
    /// find word nearest to specified point
    ldomWordEx* findNearestWord(int x, int y, MoveDirection dir);
    /// select word
    void selectWord(ldomWordEx* word, MoveDirection dir);
    /// select next word in specified direction
    ldomWordEx* selectNextWord(MoveDirection dir, int moveBy = 1);
    /// select middle word in range
    ldomWordEx* selectMiddleWord();
    /// get selected word
    ldomWordEx* getSelWord() {
        return selWord;
    }
    /// try append search pattern and find word
    ldomWordEx* appendPattern(lString32 chars);
    /// remove last character from pattern and try to search
    ldomWordEx* reducePattern();
};

#endif // __LDOMWORDEXLIST_H_INCLUDED__
