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
