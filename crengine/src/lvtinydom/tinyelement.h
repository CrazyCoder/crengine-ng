/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2010 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#ifndef __TINYELEMENT_H_INCLUDED__
#define __TINYELEMENT_H_INCLUDED__

#include <lvstyles.h>
#include <ldomdocument.h>

#include "ldomattributecollection.h"

//=====================================================
// ldomElement declaration placed here to hide DOM implementation
// use ldomNode rich interface instead
class tinyElement
{
    friend struct ldomNode;
private:
    ldomDocument* _document;
    ldomNode* _parentNode;
    lUInt16 _id;
    lUInt16 _nsid;
    LVArray<lInt32> _children;
    ldomAttributeCollection _attrs;
    lvdom_element_render_method _rendMethod;
public:
    tinyElement(ldomDocument* document, ldomNode* parentNode, lUInt16 nsid, lUInt16 id)
            : _document(document)
            , _parentNode(parentNode)
            , _id(id)
            , _nsid(nsid)
            , _rendMethod(erm_invisible) {
        _document->_tinyElementCount++;
    }
    /// destructor
    ~tinyElement() {
        _document->_tinyElementCount--;
    }
};

#endif // __TINYELEMENT_H_INCLUDED__
