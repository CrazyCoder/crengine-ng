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

#ifndef __LDOMWORD_H_INCLUDED__
#define __LDOMWORD_H_INCLUDED__

#include <ldomxpointer.h>

/// range for word inside text node
class ldomWord
{
    ldomNode* _node;
    int _start;
    int _end;
public:
    ldomWord()
            : _node(NULL)
            , _start(0)
            , _end(0) { }
    ldomWord(ldomNode* node, int start, int end)
            : _node(node)
            , _start(start)
            , _end(end) { }
    ldomWord(const ldomWord& v)
            : _node(v._node)
            , _start(v._start)
            , _end(v._end) { }
    ldomWord& operator=(const ldomWord& v) {
        _node = v._node;
        _start = v._start;
        _end = v._end;
        return *this;
    }
    /// returns true if object doesn't point valid word
    bool isNull() {
        return _node == NULL || _start < 0 || _end <= _start;
    }
    /// get word text node pointer
    ldomNode* getNode() const {
        return _node;
    }
    /// get word start offset
    int getStart() const {
        return _start;
    }
    /// get word end offset
    int getEnd() const {
        return _end;
    }
    /// get word start XPointer
    ldomXPointer getStartXPointer() const {
        return ldomXPointer(_node, _start);
    }
    /// get word start XPointer
    ldomXPointer getEndXPointer() const {
        return ldomXPointer(_node, _end);
    }
    /// get word text
    lString32 getText() {
        if (isNull())
            return lString32::empty_str;
        lString32 txt = _node->getText();
        return txt.substr(_start, _end - _start);
    }
};

#endif // __LDOMWORD_H_INCLUDED__
