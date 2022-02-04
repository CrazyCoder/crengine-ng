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

#ifndef __LDOMTEXTNODE_H_INCLUDED__
#define __LDOMTEXTNODE_H_INCLUDED__

#include <lvstring.h>

/// mutable text node
class ldomTextNode
{
    lUInt32 _parentIndex;
    lString8 _text;
public:
    lUInt32 getParentIndex() {
        return _parentIndex;
    }

    void setParentIndex(lUInt32 n) {
        _parentIndex = n;
    }

    ldomTextNode(lUInt32 parentIndex, const lString8& text)
            : _parentIndex(parentIndex)
            , _text(text) {
    }

    lString8 getText() {
        return _text;
    }

    lString32 getText32() {
        return Utf8ToUnicode(_text);
    }

    void setText(const lString8& s) {
        _text = s;
    }

    void setText(const lString32& s) {
        _text = UnicodeToUtf8(s);
    }
};

#endif // __LDOMTEXTNODE_H_INCLUDED__
