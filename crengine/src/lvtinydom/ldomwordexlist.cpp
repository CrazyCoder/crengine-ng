/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2011,2012 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#include <ldomwordexlist.h>

/// adds all visible words from range, returns number of added words
int ldomWordExList::addRangeWords(ldomXRange& range, bool /*trimPunctuation*/) {
    LVArray<ldomWord> list;
    range.getRangeWords(list);
    for (int i = 0; i < list.length(); i++)
        add(new ldomWordEx(list[i]));
    init();
    return list.length();
}

/// select word
void ldomWordExList::selectWord(ldomWordEx* word, MoveDirection dir) {
    selWord = word;
    if (selWord) {
        lvPoint middle = word->getMark().getMiddlePoint();
        if (x == -1 || (dir != DIR_UP && dir != DIR_DOWN))
            x = middle.x;
        y = middle.y;
    } else {
        x = y = -1;
    }
}

/// select next word in specified direction
ldomWordEx* ldomWordExList::selectNextWord(MoveDirection dir, int moveBy) {
    if (!selWord)
        return selectMiddleWord();
    pattern.clear();
    for (int i = 0; i < moveBy; i++) {
        ldomWordEx* word = findNearestWord(x, y, dir);
        if (word)
            selectWord(word, dir);
    }
    return selWord;
}

/// select middle word in range
ldomWordEx* ldomWordExList::selectMiddleWord() {
    if (minx == -1)
        init();
    ldomWordEx* word = findNearestWord((maxx + minx) / 2, (miny + maxy) / 2, DIR_ANY);
    selectWord(word, DIR_ANY);
    return word;
}

ldomWordEx* ldomWordExList::findWordByPattern() {
    ldomWordEx* lastBefore = NULL;
    ldomWordEx* firstAfter = NULL;
    bool selReached = false;
    for (int i = 0; i < length(); i++) {
        ldomWordEx* item = get(i);
        if (item == selWord)
            selReached = true;
        lString32 text = item->getText();
        text.lowercase();
        bool flg = true;
        for (int j = 0; j < pattern.length(); j++) {
            if (j >= text.length()) {
                flg = false;
                break;
            }
            lString32 chars = pattern[j];
            chars.lowercase();
            bool charFound = false;
            for (int k = 0; k < chars.length(); k++) {
                if (chars[k] == text[j]) {
                    charFound = true;
                    break;
                }
            }
            if (!charFound) {
                flg = false;
                break;
            }
        }
        if (!flg)
            continue;
        if (selReached) {
            if (firstAfter == NULL)
                firstAfter = item;
        } else {
            lastBefore = item;
        }
    }

    if (firstAfter)
        return firstAfter;
    else
        return lastBefore;
}

/// try append search pattern and find word
ldomWordEx* ldomWordExList::appendPattern(lString32 chars) {
    pattern.add(chars);
    ldomWordEx* foundWord = findWordByPattern();

    if (foundWord) {
        selectWord(foundWord, DIR_ANY);
    } else {
        pattern.erase(pattern.length() - 1, 1);
    }
    return foundWord;
}

/// remove last character from pattern and try to search
ldomWordEx* ldomWordExList::reducePattern() {
    if (pattern.length() == 0)
        return NULL;
    pattern.erase(pattern.length() - 1, 1);
    ldomWordEx* foundWord = findWordByPattern();

    if (foundWord)
        selectWord(foundWord, DIR_ANY);
    return foundWord;
}

/// find word nearest to specified point
ldomWordEx* ldomWordExList::findNearestWord(int x, int y, MoveDirection dir) {
    if (!length())
        return NULL;
    int bestDistance = -1;
    ldomWordEx* bestWord = NULL;
    ldomWordEx* defWord = (dir == DIR_LEFT || dir == DIR_UP) ? get(length() - 1) : get(0);
    int i;
    if (dir == DIR_LEFT || dir == DIR_RIGHT) {
        int thisLineY = -1;
        int thisLineDy = -1;
        for (i = 0; i < length(); i++) {
            ldomWordEx* item = get(i);
            lvPoint middle = item->getMark().getMiddlePoint();
            int dy = middle.y - y;
            if (dy < 0)
                dy = -dy;
            if (thisLineY == -1 || thisLineDy > dy) {
                thisLineY = middle.y;
                thisLineDy = dy;
            }
        }
        ldomWordEx* nextLineWord = NULL;
        for (i = 0; i < length(); i++) {
            ldomWordEx* item = get(i);
            if (dir != DIR_ANY && item == selWord)
                continue;
            ldomMarkedRange* mark = &item->getMark();
            lvPoint middle = mark->getMiddlePoint();
            switch (dir) {
                case DIR_LEFT:
                    if (middle.y < thisLineY)
                        nextLineWord = item; // last word of prev line
                    if (middle.x >= x)
                        continue;
                    break;
                case DIR_RIGHT:
                    if (nextLineWord == NULL && middle.y > thisLineY)
                        nextLineWord = item; // first word of next line
                    if (middle.x <= x)
                        continue;
                    break;
                case DIR_UP:
                case DIR_DOWN:
                case DIR_ANY:
                    // none
                    break;
            }
            if (middle.y != thisLineY)
                continue;
            int dist = mark->calcDistance(x, y, dir);
            if (bestDistance == -1 || dist < bestDistance) {
                bestWord = item;
                bestDistance = dist;
            }
        }
        if (bestWord != NULL)
            return bestWord; // found in the same line
        if (nextLineWord != NULL)
            return nextLineWord;
        return defWord;
    }
    for (i = 0; i < length(); i++) {
        ldomWordEx* item = get(i);
        if (dir != DIR_ANY && item == selWord)
            continue;
        ldomMarkedRange* mark = &item->getMark();
        lvPoint middle = mark->getMiddlePoint();
        if (dir == DIR_UP && middle.y >= y)
            continue;
        if (dir == DIR_DOWN && middle.y <= y)
            continue;

        int dist = mark->calcDistance(x, y, dir);
        if (bestDistance == -1 || dist < bestDistance) {
            bestWord = item;
            bestDistance = dist;
        }
    }
    if (bestWord != NULL)
        return bestWord;
    return defWord;
}

void ldomWordExList::init() {
    if (!length())
        return;
    for (int i = 0; i < length(); i++) {
        ldomWordEx* item = get(i);
        lvPoint middle = item->getMark().getMiddlePoint();
        if (i == 0 || minx > middle.x)
            minx = middle.x;
        if (i == 0 || maxx < middle.x)
            maxx = middle.x;
        if (i == 0 || miny > middle.y)
            miny = middle.y;
        if (i == 0 || maxy < middle.y)
            maxy = middle.y;
    }
}
