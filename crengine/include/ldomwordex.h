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

#ifndef __LDOMWORDEX_H_INCLUDED__
#define __LDOMWORDEX_H_INCLUDED__

#include <ldomword.h>
#include <ldommarkedrange.h>
#include <ldomxrange.h>

class ldomWordEx: public ldomWord
{
    ldomWord _word;
    ldomMarkedRange _mark;
    ldomXRange _range;
    lString32 _text;
public:
    ldomWordEx(ldomWord& word)
            : _word(word)
            , _mark(word)
            , _range(word) {
        _text = removeSoftHyphens(_word.getText());
    }
    ldomWord& getWord() {
        return _word;
    }
    ldomXRange& getRange() {
        return _range;
    }
    ldomMarkedRange& getMark() {
        return _mark;
    }
    lString32& getText() {
        return _text;
    }
};

#endif // __LDOMWORDEX_H_INCLUDED__
