/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2008,2009,2012 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2011 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2020 poire-z <poire-z@users.noreply.github.com>         *
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
