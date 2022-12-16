/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007,2009 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2018 poire-z <poire-z@users.noreply.github.com>         *
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

#ifndef __LXMLATTRIBUTE_H_INCLUDED__
#define __LXMLATTRIBUTE_H_INCLUDED__

#include <lvtinydom_common.h>

struct lxmlAttribute
{
    //
    lUInt16 nsid;
    lUInt16 id;
    lUInt32 index;
    inline bool compare(lUInt16 nsId, lUInt16 attrId) {
        return (nsId == nsid || nsId == LXML_NS_ANY) && (id == attrId);
    }
    inline void setData(lUInt16 nsId, lUInt16 attrId, lUInt32 valueIndex) {
        nsid = nsId;
        id = attrId;
        index = valueIndex;
    }
};

#endif // __LXMLATTRIBUTE_H_INCLUDED__
