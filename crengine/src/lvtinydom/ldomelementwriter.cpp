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

#include "ldomelementwriter.h"

#include <ldomdocument.h>
#include <lvdocprops.h>
#include <crbookformats.h>
#include <fb2def.h>

#include "../lvxml/lvxmlutils.h"

extern bool IS_FIRST_BODY; // declared in lvdocumentwriter.cpp

ldomElementWriter::ldomElementWriter(ldomDocument* document, lUInt16 nsid, lUInt16 id, ldomElementWriter* parent, bool insert_before_last_child)
        : _parent(parent)
        , _document(document)
        , _tocItem(NULL)
        , _isBlock(true)
        , _isSection(false)
        , _stylesheetIsSet(false)
        , _bodyEnterCalled(false)
        , _pseudoElementAfterChildIndex(-1) {
    //logfile << "{c";
    _typeDef = _document->getElementTypePtr(id);
    _flags = 0;
    if ((_typeDef && _typeDef->white_space >= css_ws_pre_line) || (_parent && _parent->getFlags() & TXTFLG_PRE))
        _flags |= TXTFLG_PRE; // Parse as PRE: pre-line, pre, pre-wrap and break-spaces
    // This will be updated in ldomElementWriter::onBodyEnter() after we have
    // set styles to this node, so we'll get the real white_space value to use.

    _isSection = (id == el_section);

#if MATHML_SUPPORT == 1
    _insideMathML = (_parent && _parent->_insideMathML) || (id == el_math);
#endif

    // Default (for elements not specified in fb2def.h) is to allow text
    // (except for the root node which must have children)
    _allowText = _typeDef ? _typeDef->allow_text : (_parent ? true : false);
    if (_document->getDOMVersionRequested() < 20180528) { // revert what was changed 20180528
        // <hr>, <ul>, <ol>, <dl>, <output>, <section>, <svg> didn't allow text
        if (id == el_hr || id == el_ul || id == el_ol || id == el_dl ||
            id == el_output || id == el_section || id == el_svg) {
            _allowText = false;
        }
        // <code> was white-space: pre
        if (id == el_code) {
            _flags |= TXTFLG_PRE;
        }
    }

    if (_parent) {
        lUInt32 index = _parent->getElement()->getChildCount();
        if (insert_before_last_child)
            index--;
        _element = _parent->getElement()->insertChildElement(index, nsid, id);
    } else
        _element = _document->getRootNode(); //->insertChildElement( (lUInt32)-1, nsid, id );
    if (id == el_body) {
        if (IS_FIRST_BODY) {
            _tocItem = _document->getToc();
            //_tocItem->clear();
            IS_FIRST_BODY = false;
        } else {
            int fmt = _document->getProps()->getIntDef(DOC_PROP_FILE_FORMAT_ID, doc_format_none);
            if (fmt == doc_format_fb2 || fmt == doc_format_fb3) {
                // Add FB2 2nd++ BODYs' titles (footnotes and endnotes) in the TOC
                // (but not their own children that are <section>)
                _isSection = true; // this is just to have updateTocItem() called
                // Also add the "NonLinear" attribute so these other BODYs are flagged
                // as non-linear and can be hidden by frontend code that handles this
                // (this is actually suggested by the FB2 specs: "... multiple
                // bodies are used for additional information, like footnotes,
                // that do not appear in the main book flow. The first body is
                // presented to the reader by default, and content in the other
                // bodies should be accessible by hyperlinks.")
                addAttribute(0, attr_NonLinear, U"");
            }
        }
    }
    //logfile << "}";
}

lUInt32 ldomElementWriter::getFlags() {
    return _flags;
}

lString32 ldomElementWriter::getPath() {
    if (!_path.empty() || _element->isRoot())
        return _path;
    _path = _parent->getPath() + "/" + _element->getXPathSegment();
    return _path;
}

static lString32 getSectionHeader(ldomNode* section) {
    lString32 header;
    if (!section || section->getChildCount() == 0)
        return header;
    ldomNode* child = section->getChildElementNode(0, U"title");
    if (!child)
        return header;
    header = child->getText(U' ', 1024);
    return header;
}

static bool isBlockNode(ldomNode* node) {
    if (!node->isElement())
        return false;
#if BUILD_LITE != 1
    if (node->getStyle()->display <= css_d_inline || node->getStyle()->display == css_d_none) {
        return false;
    }
    return true;
#else
    return true;
#endif
}

void ldomElementWriter::updateTocItem() {
    if (!_isSection)
        return;
    if (!_parent)
        return;
    if (_parent->_tocItem) { // <section> in the first <body>
        lString32 title = getSectionHeader(_element);
        //CRLog::trace("TOC ITEM: %s", LCSTR(title));
        _tocItem = _parent->_tocItem->addChild(title, ldomXPointer(_element, 0), getPath());
    } else if (getElement()->getNodeId() == el_body) { // 2nd, 3rd... <body>, in FB2 documents
        lString32 title = getSectionHeader(_element);
        _document->getToc()->addChild(title, ldomXPointer(_element, 0), getPath());
    }
    _isSection = false;
}

void ldomElementWriter::onBodyEnter() {
    _bodyEnterCalled = true;
#if BUILD_LITE != 1
    //CRLog::trace("onBodyEnter() for node %04x %s", _element->getDataIndex(), LCSTR(_element->getNodeName()));
    if (_document->isDefStyleSet() && _element) {
        _element->initNodeStyle();
        //        if ( _element->getStyle().isNull() ) {
        //            CRLog::error("error while style initialization of element %x %s", _element->getNodeIndex(), LCSTR(_element->getNodeName()) );
        //            crFatalError();
        //        }
        int nb_children = _element->getChildCount();
        if (nb_children > 0) {
            // The only possibility for this element being built to have children
            // is if the above initNodeStyle() has applied to this node some
            // matching selectors that had ::before or ::after, which have then
            // created one or two pseudoElem children. But let's be sure of that.
            for (int i = 0; i < nb_children; i++) {
                ldomNode* child = _element->getChildNode(i);
                if (child->getNodeId() == el_pseudoElem) {
                    if (child->hasAttribute(attr_Before)) {
                        // The "Before" pseudo element (not part of the XML)
                        // needs to have its style applied. As it has no
                        // children, we can also init its rend method.
                        child->initNodeStyle();
                        child->initNodeRendMethod();
                    } else if (child->hasAttribute(attr_After)) {
                        // For the "After" pseudo element, we need to wait
                        // for all real children to be added, to move it
                        // as its right position (last), to init its style
                        // (because of "content:close-quote", whose nested
                        // level need to have seen all previous nodes to
                        // be accurate) and its rendering method.
                        // We'll do that in onBodyExit() when called for
                        // this node.
                        _pseudoElementAfterChildIndex = i;
                    }
                }
            }
        }
        _isBlock = isBlockNode(_element);
        // If initNodeStyle() has set "white-space: pre" or alike, update _flags
        if (_element->getStyle()->white_space >= css_ws_pre_line) {
            _flags |= TXTFLG_PRE;
        } else {
            _flags &= ~TXTFLG_PRE;
        }
    } else {
    }
    if (_isSection) {
        if (_parent && _parent->_isSection) {
            _parent->updateTocItem();
        }
    }
#endif
}

void ldomElementWriter::onBodyExit() {
    if (_isSection)
        updateTocItem();

#if BUILD_LITE != 1
    if (!_document->isDefStyleSet())
        return;
    if (!_bodyEnterCalled) {
        onBodyEnter();
    }
    if (_pseudoElementAfterChildIndex >= 0) {
        if (_pseudoElementAfterChildIndex != _element->getChildCount() - 1) {
            // Not the last child: move it there
            // (moveItemsTo() works just fine when the source node is also the
            // target node: remove it, and re-add it, so, adding it at the end)
            _element->moveItemsTo(_element, _pseudoElementAfterChildIndex, _pseudoElementAfterChildIndex);
        }
        // Now that all the real children of this node have had their
        // style set, we can init the style of the "After" pseudo
        // element, and its rend method as it has no children.
        ldomNode* child = _element->getChildNode(_element->getChildCount() - 1);
        child->initNodeStyle();
        child->initNodeRendMethod();
    }
    //    if ( _element->getStyle().isNull() ) {
    //        lString32 path;
    //        ldomNode * p = _element->getParentNode();
    //        while (p) {
    //            path = p->getNodeName() + U"/" + path;
    //            p = p->getParentNode();
    //        }
    //        //CRLog::error("style not initialized for element 0x%04x %s path %s", _element->getDataIndex(), LCSTR(_element->getNodeName()), LCSTR(path));
    //        crFatalError();
    //    }
    _element->initNodeRendMethod();

    if (_stylesheetIsSet)
        _document->getStyleSheet()->pop();
#endif
}

void ldomElementWriter::onText(const lChar32* text, int len, lUInt32, bool insert_before_last_child) {
    //logfile << "{t";
    {
        // normal mode: store text copy
        // add text node, if not first empty space string of block node
        if (!_isBlock || _element->getChildCount() != 0 || !IsEmptySpace(text, len) || (_flags & TXTFLG_PRE)) {
            lString8 s8 = UnicodeToUtf8(text, len);
            _element->insertChildText(s8, insert_before_last_child);
        } else {
            //CRLog::trace("ldomElementWriter::onText: Ignoring first empty space of block item");
        }
    }
    //logfile << "}";
}

void ldomElementWriter::addAttribute(lUInt16 nsid, lUInt16 id, const lChar32* value) {
    getElement()->setAttributeValue(nsid, id, value);
#if BUILD_LITE != 1
    /* This is now done by ldomDocumentFragmentWriter::OnTagOpen() directly,
     * as we need to do it too for <DocFragment><stylesheet> tag, and not
     * only for <DocFragment StyleSheet="path_to_css_1st_file"> attribute.
    if ( id==attr_StyleSheet ) {
        _stylesheetIsSet = _element->applyNodeStylesheet();
    }
    */
#endif
}

ldomElementWriter::~ldomElementWriter() {
    //CRLog::trace("~ldomElementWriter for element 0x%04x %s", _element->getDataIndex(), LCSTR(_element->getNodeName()));
    //getElement()->persist();
    onBodyExit();
}
