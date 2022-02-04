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

#ifndef __LDOMELEMENTWRITER_H_INCLUDED__
#define __LDOMELEMENTWRITER_H_INCLUDED__

#include <lvstring.h>

class ldomDocument;
struct ldomNode;
class LVTocItem;
struct css_elem_def_props_t;

class ldomElementWriter
{
    ldomElementWriter* _parent;
    ldomDocument* _document;

    ldomNode* _element;
    LVTocItem* _tocItem;
    lString32 _path;
    const css_elem_def_props_t* _typeDef;
    bool _allowText;
    bool _isBlock;
    bool _isSection;
#if MATHML_SUPPORT == 1
    bool _insideMathML;
#endif
    bool _stylesheetIsSet;
    bool _bodyEnterCalled;
    int _pseudoElementAfterChildIndex;
    lUInt32 _flags;
    lUInt32 getFlags();
    void updateTocItem();
    void onBodyEnter();
    void onBodyExit();
    ldomNode* getElement() {
        return _element;
    }
    lString32 getPath();
    void onText(const lChar32* text, int len, lUInt32 flags, bool insert_before_last_child = false);
    void addAttribute(lUInt16 nsid, lUInt16 id, const lChar32* value);
    //lxmlElementWriter * pop( lUInt16 id );

    ldomElementWriter(ldomDocument* document, lUInt16 nsid, lUInt16 id, ldomElementWriter* parent, bool insert_before_last_child = false);
    ~ldomElementWriter();

    friend class ldomDocumentWriter;
    friend class ldomDocumentWriterFilter;
#if MATHML_SUPPORT == 1
    friend class MathMLHelper;
#endif
    //friend ldomElementWriter * pop( ldomElementWriter * obj, lUInt16 id );
};

#endif // __LDOMELEMENTWRITER_H_INCLUDED__
