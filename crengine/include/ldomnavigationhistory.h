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

#ifndef __LDOMNAVIGATIONHISTORY_H_INCLUDED__
#define __LDOMNAVIGATIONHISTORY_H_INCLUDED__

#include <lvstring32collection.h>

class ldomNavigationHistory
{
private:
    lString32Collection _links;
    int _pos;
    void clearTail() {
        if (_links.length() > _pos)
            _links.erase(_pos, _links.length() - _pos);
    }
public:
    void clear() {
        _links.clear();
        _pos = 0;
    }
    bool save(lString32 link) {
        if (_pos == (int)_links.length() && _pos > 0 && _links[_pos - 1] == link)
            return false;
        if (_pos >= (int)_links.length() || _links[_pos] != link) {
            clearTail();
            _links.add(link);
            _pos = _links.length();
            return true;
        } else if (_links[_pos] == link) {
            _pos++;
            return true;
        }
        return false;
    }
    lString32 back() {
        if (_pos == 0)
            return lString32::empty_str;
        return _links[--_pos];
    }
    lString32 forward() {
        if (_pos >= (int)_links.length() - 1)
            return lString32::empty_str;
        return _links[++_pos];
    }
    int backCount() {
        return _pos;
    }
    int forwardCount() {
        return _links.length() - _pos;
    }
};

#endif // __LDOMNAVIGATIONHISTORY_H_INCLUDED__
