/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007-2012 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2013,2019,2020 Konstantin Potapov <pkbo@users.sourceforge.net>
 *   Copyright (C) 2015-2017 Yifei(Frank) ZHU <fredyifei@gmail.com>        *
 *   Copyright (C) 2018 Frans de Jonge <fransdejonge@gmail.com>            *
 *   Copyright (C) 2021 ourairquality <info@ourairquality.org>             *
 *   Copyright (C) 2018-2021 Aleksey Chernov <valexlin@gmail.com>          *
 *   Copyright (C) 2017-2021 poire-z <poire-z@users.noreply.github.com>    *
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

#include <ldomnode.h>
#include <lvdocviewcallback.h>
#include <fb2def.h>
#include <lvrend.h>
#include <crlog.h>

#include "lvtinydom_private.h"
#include "lxmlattribute.h"
#include "ldomtextnode.h"
#include "tinyelement.h"
#include "ldomdatastoragemanager.h"
#include "datastorageitem.h"
#include "lvbase64nodestream.h"
#include "nodeimageproxy.h"
#include "renderrectaccessor.h"
#include "../textlang.h"

#if MATHML_SUPPORT == 1
#include "../mathml.h"
#endif

// define to store new text nodes as persistent text, instead of mutable
#define USE_PERSISTENT_TEXT 1

//#define TRACE_AUTOBOX

// shortcut for dynamic element accessor
#ifdef _DEBUG
#define ASSERT_NODE_NOT_NULL \
    if (isNull())            \
    crFatalError(1313, "Access to null node")
#else
#define ASSERT_NODE_NOT_NULL
#endif

#define NPELEM _data._elem_ptr
#define NPTEXT _data._text_ptr._str

static bool isInlineNode(ldomNode* node) {
    if (node->isText())
        return true;
    //int d = node->getStyle()->display;
    //return ( d==css_d_inline || d==css_d_run_in );
    int m = node->getRendMethod();
    return m == erm_inline;
}

static bool isFloatingNode(ldomNode* node) {
    if (node->isText())
        return false;
    return node->getStyle()->float_ > css_f_none;
}

static bool isNotBoxWrappingNode(ldomNode* node) {
    if (BLOCK_RENDERING_N(node, PREPARE_FLOATBOXES) && node->getStyle()->float_ > css_f_none)
        return false; // floatBox
    // isBoxingInlineBox() already checks for BLOCK_RENDERING_BOX_INLINE_BLOCKS)
    return !node->isBoxingInlineBox();
}

static bool isNotBoxingInlineBoxNode(ldomNode* node) {
    return !node->isBoxingInlineBox();
}

static void resetRendMethodToInline(ldomNode* node) {
    // we shouldn't reset to inline (visible) if display: none
    // (using node->getRendMethod() != erm_invisible seems too greedy and may
    // hide other nodes)
    if (node->getStyle()->display != css_d_none)
        node->setRendMethod(erm_inline);
    else if (node->getDocument()->getDOMVersionRequested() < 20180528) // do that in all cases
        node->setRendMethod(erm_inline);
}

// commented in initNodeRendMethod()
// TODO: remove method resetRendMethodToInvisible() & clean initNodeRendMethod() ?
#if 0
static void resetRendMethodToInvisible(ldomNode* node) {
    node->setRendMethod(erm_invisible);
}
#endif

static bool hasInvisibleParent(ldomNode* node) {
    for (; node && !node->isRoot(); node = node->getParentNode()) {
        css_style_ref_t style = node->getStyle();
        if (!style.isNull() && style->display == css_d_none)
            return true;
    }
    return false;
}

// Uncomment to debug COMPLETE_INCOMPLETE_TABLES tabularBox wrapping
// #define DEBUG_INCOMPLETE_TABLE_COMPLETION

// init table element render methods
// states: 0=table, 1=colgroup, 2=rowgroup, 3=row, 4=cell
// returns table cell count
// When BLOCK_RENDERING_COMPLETE_INCOMPLETE_TABLES, we follow rules
// from the "Generate missing child wrappers" section in:
//   https://www.w3.org/TR/CSS22/tables.html#anonymous-boxes
//   https://www.w3.org/TR/css-tables-3/#fixup (clearer than previous one)
// and we wrap unproper children in a tabularBox element.
static int initTableRendMethods(ldomNode* enode, int state) {
    //main node: table
    if (state == 0 && (enode->getStyle()->display == css_d_table ||
                       enode->getStyle()->display == css_d_inline_table ||
                       (enode->getStyle()->display == css_d_inline_block && enode->getNodeId() == el_table))) {
        enode->setRendMethod(erm_table);
    }
    int cellCount = 0; // (returned, but not used anywhere)
    int cnt = enode->getChildCount();
    int i;
    int first_unproper = -1; // keep track of consecutive unproper children that
    int last_unproper = -1;  // must all be wrapped in a single wrapper
    for (i = 0; i < cnt; i++) {
        ldomNode* child = enode->getChildNode(i);
        css_display_t d;
        if (child->isElement()) {
            d = child->getStyle()->display;
        } else { // text node
            d = css_d_inline;
            // Not sure about what to do with whitespace only text nodes:
            // we shouldn't meet any alongside real elements (as whitespace
            // around and at start/end of block nodes are discarded), but
            // we may in case of style changes (inline > table) after
            // a book has been loaded.
            // Not sure if we should handle them differently when no unproper
            // elements yet (they will be discarded by the table render algo),
            // and when among unpropers (they could find their place in the
            // wrapped table cell).
            // Note that boxWrapChildren() called below will remove
            // them at start or end of an unproper elements sequence.
        }
        bool is_last = (i == cnt - 1);
        bool is_proper = false;
        if (state == 0) { // in table
            if (d == css_d_table_row) {
                child->setRendMethod(erm_table_row);
                cellCount += initTableRendMethods(child, 3); // > row
                is_proper = true;
            } else if (d == css_d_table_row_group) {
                child->setRendMethod(erm_table_row_group);
                cellCount += initTableRendMethods(child, 2); // > rowgroup
                is_proper = true;
            } else if (d == css_d_table_header_group) {
                child->setRendMethod(erm_table_header_group);
                cellCount += initTableRendMethods(child, 2); // > rowgroup
                is_proper = true;
            } else if (d == css_d_table_footer_group) {
                child->setRendMethod(erm_table_footer_group);
                cellCount += initTableRendMethods(child, 2); // > rowgroup
                is_proper = true;
            } else if (d == css_d_table_column_group) {
                child->setRendMethod(erm_table_column_group);
                cellCount += initTableRendMethods(child, 1); // > colgroup
                is_proper = true;
            } else if (d == css_d_table_column) {
                child->setRendMethod(erm_table_column);
                is_proper = true;
            } else if (d == css_d_table_caption) {
                child->setRendMethod(erm_final);
                is_proper = true;
            } else if (d == css_d_none) {
                child->setRendMethod(erm_invisible);
                is_proper = true;
            } else if (child->getNodeId() == el_tabularBox) {
// Most probably added by us in a previous rendering
#ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                printf("initTableRendMethods(0): (reused)wrapping unproper > row\n");
#endif
                child->setRendMethod(erm_table_row);
                cellCount += initTableRendMethods(child, 3); // > row
                is_proper = true;
            }
        } else if (state == 2) { // in rowgroup
            if (d == css_d_table_row) {
                child->setRendMethod(erm_table_row);
                cellCount += initTableRendMethods(child, 3); // > row
                is_proper = true;
            } else if (d == css_d_none) {
                child->setRendMethod(erm_invisible);
                is_proper = true;
            } else if (child->getNodeId() == el_tabularBox) {
// Most probably added by us in a previous rendering
#ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                printf("initTableRendMethods(2): (reused)wrapping unproper > row\n");
#endif
                child->setRendMethod(erm_table_row);
                cellCount += initTableRendMethods(child, 3); // > row
                is_proper = true;
            }
        } else if (state == 3) { // in row
            if (d == css_d_table_cell) {
                // This will set the rend method of the cell to either erm_block
                // or erm_final, depending on its content.
                child->initNodeRendMethodRecursive();
                cellCount++;
                is_proper = true;
            } else if (d == css_d_none) {
                child->setRendMethod(erm_invisible);
                is_proper = true;
            } else if (child->getNodeId() == el_tabularBox) {
// Most probably added by us in a previous rendering
#ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                printf("initTableRendMethods(3): (reused)wrapping unproper > cell\n");
#endif
                // This will set the rend method of the cell to either erm_block \
        // or erm_final, depending on its content.
                child->initNodeRendMethodRecursive();
                cellCount++;
                is_proper = true;
            }
        } else if (state == 1) { // in colgroup
            if (d == css_d_table_column) {
                child->setRendMethod(erm_table_column);
                is_proper = true;
            } else {
                // No need to tabularBox invalid colgroup children:
                // they are not rendered, and should be considered
                // as if display: none.
                child->setRendMethod(erm_invisible);
                is_proper = true;
            }
        } else { // shouldn't be reached
            crFatalError(151, "initTableRendMethods state unexpected");
            // child->setRendMethod( erm_final );
        }

        // Check and deal with unproper children
        if (!is_proper) { // Unproper child met
            // printf("initTableRendMethods(%d): child %d is unproper\n", state, i);
            lUInt32 rend_flags = enode->getDocument()->getRenderBlockRenderingFlags();
            if (BLOCK_RENDERING(rend_flags, COMPLETE_INCOMPLETE_TABLES)) {
                // We can insert a tabularBox element to wrap unproper elements
                last_unproper = i;
                if (first_unproper < 0)
                    first_unproper = i;
            } else {
                // Asked to not complete incomplete tables, or we can't insert
                // tabularBox elements anymore
                if (!BLOCK_RENDERING(rend_flags, ENHANCED)) {
                    // Legacy behaviour was to just make invisible internal-table
                    // elements that were not found in their proper internal-table
                    // container, but let other non-internal-table elements be
                    // (which might be rendered and drawn quite correctly when
                    // they are erm_final/erm_block, but won't be if erm_inline).
                    if (d > css_d_table) {
                        child->setRendMethod(erm_invisible);
                    }
                } else {
                    // When in enhanced mode, we let the ones that could
                    // be rendered and drawn quite correctly be. But we'll
                    // have the others drawn as erm_killed, showing a small
                    // symbol so users know some content is missing.
                    if (d > css_d_table || d <= css_d_inline) {
                        child->setRendMethod(erm_killed);
                    }
                    // Note that there are other situations where some content
                    // would not be shown when !COMPLETE_INCOMPLETE_TABLES, and
                    // for which we are not really able to set some node as
                    // erm_killed (for example, with TABLE > TABLE, the inner
                    // one will be rendered, but the outer one would have
                    // a height=0, and so the inner content will overflow
                    // its container and will not be drawn...)
                }
            }
        }
        if (first_unproper >= 0 && (is_proper || is_last)) {
// We met unproper children, but we now have a proper child, or we're done:
// wrap all these consecutive unproper nodes inside a single tabularBox
// element with the proper rendering method.
#ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
            printf("initTableRendMethods(%d): wrapping unproper %d>%d\n",
                   state, first_unproper, last_unproper);
#endif
            int elems_removed = last_unproper - first_unproper + 1;
            ldomNode* tbox = enode->boxWrapChildren(first_unproper, last_unproper, el_tabularBox);
            if (tbox && !tbox->isNull()) {
                elems_removed -= 1;             // tabularBox added
                if (state == 0 || state == 2) { // in table or rowgroup
                    // No real need to store the style as an attribute: it would
                    // be remembered and re-used when styles change, and just
                    // setting the appropriate rendering method is all that is
                    // needed for rendering after this.
                    // tbox->setAttributeValue(LXML_NS_NONE, enode->getDocument()->getAttrNameIndex(U"style"), U"display: table-row");
                    tbox->initNodeStyle();
                    tbox->setRendMethod(erm_table_row);
                    cellCount += initTableRendMethods(tbox, 3); // > row
                } else if (state == 3) {
                    tbox->initNodeStyle();
                    // This will set the rend method of the cell to either erm_block
                    // or erm_final, depending on its content.
                    tbox->initNodeRendMethodRecursive();
                    cellCount++;
                } else if (state == 1) { // should not happen, see above
                    tbox->initNodeStyle();
                    tbox->setRendMethod(erm_table_column);
                }
            }
            // If tbox is NULL, all unproper have been removed, and no element added
            if (is_last)
                break;
            // Account for what's been removed in our loop index and end
            i -= elems_removed;
            cnt -= elems_removed;
            first_unproper = -1;
            last_unproper = -1;
        }
        child->persist();
    }
    // if ( state==0 ) {
    //     dumpRendMethods( enode, cs32("   ") );
    // }
    return cellCount;
}

static void detectChildTypes(ldomNode* parent, bool& hasBlockItems, bool& hasInline,
                             bool& hasInternalTableItems, bool& hasFloating, bool detectFloating = false) {
    hasBlockItems = false;
    hasInline = false;
    hasFloating = false;
    if (parent->getNodeId() == el_pseudoElem) {
        // pseudoElem (generated from CSS ::before and ::after), will have
        // some (possibly empty) plain text content.
        hasInline = true;
        return; // and it has no children
    }
    int len = parent->getChildCount();
    for (int i = len - 1; i >= 0; i--) {
        ldomNode* node = parent->getChildNode(i);
        if (!node->isElement()) {
            // text
            hasInline = true;
        } else if (detectFloating && node->getStyle()->float_ > css_f_none) {
            hasFloating = true;
        } else {
            // element
            int d = node->getStyle()->display;
            int m = node->getRendMethod();
            if (d == css_d_none || m == erm_invisible)
                continue;
            if (m == erm_inline) { //d==css_d_inline || d==css_d_run_in
                hasInline = true;
            } else {
                hasBlockItems = true;
                // (Table internal elements are all block items in the context
                // where hasBlockItems is used, so account for them in both)
                if ((d > css_d_table && d <= css_d_table_caption) || (m > erm_table)) {
                    hasInternalTableItems = true;
                }
            }
        }
    }
}

static void readOnlyError() {
    crFatalError(125, "Text node is persistent (read-only)! Call modify() to get r/w instance.");
}

static int _nextDocumentIndex = 0;
ldomDocument* ldomNode::_documentInstances[MAX_DOCUMENT_INSTANCE_COUNT] = {
    NULL,
};

/// adds document to list, returns ID of allocated document, -1 if no space in instance array
int ldomNode::registerDocument(ldomDocument* doc) {
    for (int i = 0; i < MAX_DOCUMENT_INSTANCE_COUNT; i++) {
        if (_nextDocumentIndex < 0 || _nextDocumentIndex >= MAX_DOCUMENT_INSTANCE_COUNT)
            _nextDocumentIndex = 0;
        if (_documentInstances[_nextDocumentIndex] == NULL) {
            _documentInstances[_nextDocumentIndex] = doc;
            CRLog::info("ldomNode::registerDocument() - new index = %d", _nextDocumentIndex);
            return _nextDocumentIndex++;
        }
        _nextDocumentIndex++;
    }
    return -1;
}

/// removes document from list
void ldomNode::unregisterDocument(ldomDocument* doc) {
    for (int i = 0; i < MAX_DOCUMENT_INSTANCE_COUNT; i++) {
        if (_documentInstances[i] == doc) {
            CRLog::info("ldomNode::unregisterDocument() - for index %d", i);
            _documentInstances[i] = NULL;
        }
    }
}

void ldomNode::ensurePseudoElement(bool is_before) {
    // This node should have that pseudoElement, but it might already be there,
    // so check if there is already one, and if not, create it.
    // This happens usually in the initial loading phase, but it might in
    // a re-rendering if the pseudo element is introduced by a change in
    // styles (we won't be able to create a node if there's a cache file).
    int insertChildIndex = -1;
    int nb_children = getChildCount();
    if (is_before) {          // ::before
        insertChildIndex = 0; // always to be inserted first, if not already there
        if (nb_children > 0) {
            ldomNode* child = getChildNode(0); // should always be found as the first node
            // pseudoElem might have been wrapped by a inlineBox, autoBoxing, floatBox...
            while (child && child->isBoxingNode() && child->getChildCount() > 0)
                child = child->getChildNode(0);
            if (child && child->getNodeId() == el_pseudoElem && child->hasAttribute(attr_Before)) {
                // Already there, no need to create it
                insertChildIndex = -1;
            }
        }
    } else { // ::after
        // In the XML loading phase, this one might be either first,
        // or second if there's already a Before. In the re-rendering
        // phase, it would have been moved as the last node. In all these
        // cases, it is always the last at the moment we are checking.
        insertChildIndex = nb_children; // always to be inserted last, if not already there
        if (nb_children > 0) {
            ldomNode* child = getChildNode(nb_children - 1); // should always be found as the last node
            // pseudoElem might have been wrapped by a inlineBox, autoBoxing, floatBox...
            while (child && child->isBoxingNode() && child->getChildCount() > 0)
                child = child->getChildNode(child->getChildCount() - 1);
            if (child && child->getNodeId() == el_pseudoElem && child->hasAttribute(attr_After)) {
                // Already there, no need to create it
                insertChildIndex = -1;
            }
        }
    }
    if (insertChildIndex >= 0) {
        ldomNode* pseudo = insertChildElement(insertChildIndex, LXML_NS_NONE, el_pseudoElem);
        lUInt16 attribute_id = is_before ? attr_Before : attr_After;
        pseudo->setAttributeValue(LXML_NS_NONE, attribute_id, U"");
        // We are called by lvrend.cpp setNodeStyle(), after the parent
        // style and font have been fully set up. We could set this pseudo
        // element style with pseudo->initNodeStyle(), as it can inherit
        // properly, but we should not:
        // - when re-rendering, initNodeStyleRecursive()/updateStyleDataRecursive()
        //   will iterate thru this node we just added as a child, and do it.
        // - when XML loading, we could do it for the "Before" pseudo element,
        //   but for the "After" one, we need to wait for all real children to be
        //   added and have their style applied - just because they can change
        //   open-quote/close-quote nesting levels - to be sure we get the
        //   proper nesting level quote char for the After node.
        // So, for the XML loading phase, we do that in onBodyEnter() and
        // onBodyExit() when called on the parent node.
    }
}

void ldomNode::removeChildren(int startIndex, int endIndex) {
    for (int i = endIndex; i >= startIndex; i--) {
        removeChild(i)->destroy();
    }
}

void ldomNode::autoboxChildren(int startIndex, int endIndex, bool handleFloating) {
    if (!isElement())
        return;
    css_style_ref_t style = getStyle();
    bool pre = (style->white_space >= css_ws_pre_line);
    // (css_ws_pre_line might need special care?)
    int firstNonEmpty = startIndex;
    int lastNonEmpty = endIndex;

    bool hasInline = pre;
    bool hasNonEmptyInline = pre;
    bool hasFloating = false;
    // (Note: did not check how floats inside <PRE> are supposed to work)
    if (!pre) {
        while (firstNonEmpty <= endIndex && getChildNode(firstNonEmpty)->isText()) {
            lString32 s = getChildNode(firstNonEmpty)->getText();
            if (!IsEmptySpace(s.c_str(), s.length()))
                break;
            firstNonEmpty++;
        }
        while (lastNonEmpty >= endIndex && getChildNode(lastNonEmpty)->isText()) {
            lString32 s = getChildNode(lastNonEmpty)->getText();
            if (!IsEmptySpace(s.c_str(), s.length()))
                break;
            lastNonEmpty--;
        }

        for (int i = firstNonEmpty; i <= lastNonEmpty; i++) {
            ldomNode* node = getChildNode(i);
            if (isInlineNode(node)) {
                hasInline = true;
                if (!hasNonEmptyInline) {
                    if (node->isText()) {
                        lString32 s = node->getText();
                        if (!IsEmptySpace(s.c_str(), s.length())) {
                            hasNonEmptyInline = true;
                        }
                    } else {
                        if (handleFloating && isFloatingNode(node)) {
                            // Ignore floatings
                        } else {
                            hasNonEmptyInline = true;
                            // Note: when not using DO_NOT_CLEAR_OWN_FLOATS, we might
                            // want to be more agressive in the removal of empty
                            // elements, including nested empty elements which would
                            // have no effect on the rendering (eg, some empty <link/>
                            // or <span id="PageNumber123"/>), to avoid having the float
                            // in an autoBox element with nothing else, which would
                            // then be cleared and leave some blank space.
                            // We initially did:
                            //    // For now, assume any inline node with some content
                            //    // (text or other inlines) is non empty.
                            //    if ( node->getChildCount() > 0 )
                            //        hasNonEmptyInline = true;
                            //    else if (node->getNodeId() == el_br) {
                            //        hasNonEmptyInline = true;
                            //    }
                            //    else {
                            //        const css_elem_def_props_t * ntype = node->getElementTypePtr();
                            //        if (ntype && ntype->is_object) // standalone image
                            //            hasNonEmptyInline = true;
                            //    }
                            // and we could even use hasNonEmptyInlineContent() to get
                            // rid of any nested empty elements and be sure to have our
                            // float standalone and be able to have it rendered as block
                            // instead of in an erm_final.
                            //
                            // But this was for edge cases (but really noticable), and it has
                            // become less critical now that we have/ DO_NOT_CLEAR_OWN_FLOATS,
                            // so let's not remove any element from our DOM (those with some
                            // id= attribute might be the target of a link).
                            //
                            // Sample test case in China.EN at the top of the "Politics" section:
                            //   "...</div> <link/> (or any text) <div float>...</div> <div>..."
                            // gets turned into:
                            //   "...</div>
                            //   <autoBoxing>
                            //     <link/> (or any text)
                            //     <floatBox>
                            //       <div float>...</div>
                            //     </floatBox>
                            //   </autoBoxing>
                            //   <div>..."
                            // If the floatbox would be let outside of the autobox, it would
                            // be fine when not DO_NOT_CLEAR_OWN_FLOATS too.
                        }
                    }
                }
            }
            if (handleFloating && isFloatingNode(node))
                hasFloating = true;
            if (hasNonEmptyInline && hasFloating)
                break; // We know, no need to look more
        }
    }

    if (hasFloating && !hasNonEmptyInline) {
        // only multiple floats with empty spaces in between:
        // remove empty text nodes, and let the floats be blocks, don't autobox
        for (int i = endIndex; i >= startIndex; i--) {
            if (!isFloatingNode(getChildNode(i)))
                removeChildren(i, i);
        }
    } else if (hasInline) { //&& firstNonEmpty<=lastNonEmpty

#ifdef TRACE_AUTOBOX
        CRLog::trace("Autobox children %d..%d of node <%s>  childCount=%d", firstNonEmpty, lastNonEmpty, LCSTR(getNodeName()), getChildCount());

        for (int i = firstNonEmpty; i <= lastNonEmpty; i++) {
            ldomNode* node = getChildNode(i);
            if (node->isText())
                CRLog::trace("    text: %d '%s'", node->getDataIndex(), LCSTR(node->getText()));
            else
                CRLog::trace("    elem: %d <%s> rendMode=%d  display=%d", node->getDataIndex(), LCSTR(node->getNodeName()), node->getRendMethod(), node->getStyle()->display);
        }
#endif
        // remove trailing empty
        removeChildren(lastNonEmpty + 1, endIndex);

        // inner inline
        ldomNode* abox = insertChildElement(firstNonEmpty, LXML_NS_NONE, el_autoBoxing);
        moveItemsTo(abox, firstNonEmpty + 1, lastNonEmpty + 1);
        // remove starting empty
        removeChildren(startIndex, firstNonEmpty - 1);
        abox->initNodeStyle();
        if (!BLOCK_RENDERING_N(this, FLOAT_FLOATBOXES)) {
            // If we don't want floatBoxes floating, reset them to be
            // rendered inline among inlines
            abox->recurseMatchingElements(resetRendMethodToInline, isNotBoxingInlineBoxNode);
        }
        abox->setRendMethod(erm_final);
    } else if (hasFloating) {
        // only floats, don't autobox them (otherwise the autobox wouldn't be floating)
        // remove trailing empty
        removeChildren(lastNonEmpty + 1, endIndex);
        // remove starting empty
        removeChildren(startIndex, firstNonEmpty - 1);
    } else {
        // only empty items: remove them instead of autoboxing
        removeChildren(startIndex, endIndex);
    }
}

bool ldomNode::cleanIfOnlyEmptyTextInline(bool handleFloating) {
    if (!isElement())
        return false;
    css_style_ref_t style = getStyle();
    if (style->white_space >= css_ws_pre)
        return false; // Don't mess with PRE (css_ws_pre_line might need special care?)
    // We return false as soon as we find something non text, or text non empty
    int i = getChildCount() - 1;
    for (; i >= 0; i--) {
        ldomNode* node = getChildNode(i);
        if (node->isText()) {
            lString32 s = node->getText();
            if (!IsEmptySpace(s.c_str(), s.length())) {
                return false;
            }
        } else if (handleFloating && isFloatingNode(node)) {
            // Ignore floatings
        } else { // non-text non-float element
            return false;
        }
    }
    // Ok, only empty text inlines, with possible floats
    i = getChildCount() - 1;
    for (; i >= 0; i--) {
        // With the tests done above, we just need to remove text nodes
        if (getChildNode(i)->isText()) {
            removeChildren(i, i);
        }
    }
    return true;
}

/// returns true if element has inline content (non empty text, images, <BR>)
bool ldomNode::hasNonEmptyInlineContent(bool ignoreFloats) {
    if (getRendMethod() == erm_invisible) {
        return false;
    }
    if (ignoreFloats && BLOCK_RENDERING_N(this, FLOAT_FLOATBOXES) && getStyle()->float_ > css_f_none) {
        return false;
    }
    // With some other bool param, we might want to also check for
    // padding top/bottom (and height if check ENSURE_STYLE_HEIGHT)
    // if these will introduce some content.
    if (isText()) {
        lString32 s = getText();
        return !IsEmptySpace(s.c_str(), s.length());
    }
    if (getNodeId() == el_br) {
        return true;
    }
    const css_elem_def_props_t* ntype = getElementTypePtr();
    if (ntype && ntype->is_object) { // standalone image
        return true;
    }
    for (int i = 0; i < (int)getChildCount(); i++) {
        if (getChildNode(i)->hasNonEmptyInlineContent()) {
            return true;
        }
    }
    return false;
}

// Generic version of autoboxChildren() without any specific inline/block checking,
// accepting any element id (from the enum el_*, like el_div, el_tabularBox) as
// the wrapping element.
ldomNode* ldomNode::boxWrapChildren(int startIndex, int endIndex, lUInt16 elementId) {
    if (!isElement())
        return NULL;
    int firstNonEmpty = startIndex;
    int lastNonEmpty = endIndex;

    while (firstNonEmpty <= endIndex && getChildNode(firstNonEmpty)->isText()) {
        lString32 s = getChildNode(firstNonEmpty)->getText();
        if (!IsEmptySpace(s.c_str(), s.length()))
            break;
        firstNonEmpty++;
    }
    while (lastNonEmpty >= endIndex && getChildNode(lastNonEmpty)->isText()) {
        lString32 s = getChildNode(lastNonEmpty)->getText();
        if (!IsEmptySpace(s.c_str(), s.length()))
            break;
        lastNonEmpty--;
    }

    // printf("boxWrapChildren %d>%d | %d<%d\n", startIndex, firstNonEmpty, lastNonEmpty, endIndex);
    if (firstNonEmpty <= lastNonEmpty) {
        // remove trailing empty
        removeChildren(lastNonEmpty + 1, endIndex);
        // create wrapping container
        ldomNode* box = insertChildElement(firstNonEmpty, LXML_NS_NONE, elementId);
        moveItemsTo(box, firstNonEmpty + 1, lastNonEmpty + 1);
        // remove starting empty
        removeChildren(startIndex, firstNonEmpty - 1);
        return box;
    } else {
        // Only empty items: remove them instead of box wrapping them
        removeChildren(startIndex, endIndex);
        return NULL;
    }
}

bool ldomNode::isFloatingBox() const {
    // BLOCK_RENDERING_FLOAT_FLOATBOXES is what triggers rendering
    // the floats floating. They are wrapped in a floatBox, possibly
    // not floating, when BLOCK_RENDERING_WRAP_FLOATS.
    if (BLOCK_RENDERING_N(this, FLOAT_FLOATBOXES) && getNodeId() == el_floatBox && getStyle()->float_ > css_f_none)
        return true;
    return false;
}

/// is node an inlineBox that has not been re-inlined by having
/// its child no more inline-block/inline-table
bool ldomNode::isBoxingInlineBox() const {
    // BLOCK_RENDERING_BOX_INLINE_BLOCKS) is what ensures inline-block
    // are boxed and rendered as an inline block, but we may have them
    // wrapping a node that is no more inline-block (when some style
    // tweaks have changed the display: property).
    if (getNodeId() == el_inlineBox && BLOCK_RENDERING_N(this, BOX_INLINE_BLOCKS)) {
        if (getChildCount() == 1) {
            css_display_t d = getChildNode(0)->getStyle()->display;
            if (d == css_d_inline_block || d == css_d_inline_table) {
                return true;
            }
            // Also if this box parent is <ruby> and if what this inlineBox
            // contains (probably a rubyBox) is being rendered as erm_table
            if (getChildNode(0)->getRendMethod() == erm_table && getParentNode() && getParentNode()->getStyle()->display == css_d_ruby) {
                return true;
            }
            return isEmbeddedBlockBoxingInlineBox(true); // avoid rechecking what we just checked
        }
    }
    return false;
}

/// is node an inlineBox that wraps a bogus embedded block (not inline-block/inline-table)
/// can be called with inline_box_checks_done=true when isBoxingInlineBox() has already
/// been called to avoid rechecking what is known
bool ldomNode::isEmbeddedBlockBoxingInlineBox(bool inline_box_checks_done) const {
    if (!inline_box_checks_done) {
        if (getNodeId() != el_inlineBox || !BLOCK_RENDERING_N(this, BOX_INLINE_BLOCKS))
            return false;
        if (getChildCount() != 1)
            return false;
        css_display_t d = getChildNode(0)->getStyle()->display;
        if (d == css_d_inline_block || d == css_d_inline_table) {
            return false; // regular boxing inlineBox
        }
        if (getChildNode(0)->getRendMethod() == erm_table && getParentNode() && getParentNode()->getStyle()->display == css_d_ruby) {
            return false; // inlineBox wrapping a rubyBox as a child of <ruby>
        }
    }
    if (hasAttribute(attr_T)) { // T="EmbeddedBlock"
        // (no other possible value yet, no need to compare strings)
        int cm = getChildNode(0)->getRendMethod();
        if (cm == erm_inline || cm == erm_invisible || cm == erm_killed)
            return false; // child has been reset to inline
        return true;
    }
    return false;
}

void ldomNode::initNodeRendMethod() {
    // This method is called when re-rendering, but also while
    // initially loading a document.
    // On initial loading:
    //   A node's style is defined when the node element XML tag
    //   opening is processed (by lvrend.cpp setNodeStyle() which
    //   applies inheritance from its parent, which has
    //   already been parsed).
    //   This method is called when the node element XML tag is
    //   closed, so all its children are known, have styles, and
    //   have had this method called on them.
    // On re-rendering:
    //   Styles are first applied recursively, parents first (because
    //   of inheritance).
    //   This method is then called thru recurseElementsDeepFirst, so
    //   from deepest children up to their parents up to the root node.
    // So, this method should decide how this node is going to be
    // rendered (inline, block containing other blocks, or final block
    // containing only inlines), only from the node's own style, and
    // from the styles and rendering methods of its children.
    if (!isElement())
        return;
    if (isRoot()) {
        setRendMethod(erm_block);
        return;
    }

    // DEBUG TEST
    // if ( getParentNode()->getChildIndex( getDataIndex() )<0 ) {
    //     CRLog::error("Invalid parent->child relation for nodes %d->%d", getParentNode()->getDataIndex(), getDataIndex() );
    // }
    // if ( getNodeName() == "image" ) {
    //     CRLog::trace("Init log for image");
    // }

    // Needed if COMPLETE_INCOMPLETE_TABLES, so have it updated along
    // the way to avoid an extra loop for checking if we have some.
    bool hasInternalTableItems = false;

    int d = getStyle()->display;
    lUInt32 rend_flags = getDocument()->getRenderBlockRenderingFlags();

    if (hasInvisibleParent(this)) { // (should be named isInvisibleOrHasInvisibleParent())
        // Note: we could avoid that up-to-root-node walk for each node
        // by inheriting css_d_none in setNodeStyle(), and just using
        // "if ( d==css_d_none )" instead of hasInvisibleParent(this).
        // But not certain this would have no side effect, and some
        // quick tests show no noticeable change in rendering timing.
        //
        //recurseElements( resetRendMethodToInvisible );
        setRendMethod(erm_invisible);
    } else if (d == css_d_inline) {
        // Used to be: an inline parent resets all its children to inline
        //   (so, if some block content is erroneously wrapped in a SPAN, all
        //   the content became inline...), except, depending on what's enabled:
        //   - nodes with float: which can stay block among inlines
        //   - the inner content of inlineBoxes (the inlineBox is already inline)
        //   recurseMatchingElements( resetRendMethodToInline, isNotBoxWrappingNode );
        //
        // But we don't want to "reset all its children to inline" when a bogus
        // spurious block element happens to be inside some inline one, as this
        // can be seen happening (<small> multiple <p>...</small>).
        // So, when BOX_INLINE_BLOCKS is enabled, we wrap such block elements inside
        // a <inlineBox> element, nearly just like if it were "display: inline-block",
        // with a few tweaks in its rendering (see below).
        // Or, if it contains only block elements, and empty text nodes, we can just
        // set this inline element to be erm_block.
        //
        // Some discussions about that "block inside inline" at:
        //   https://github.com/w3c/csswg-drafts/issues/1477
        //   https://stackoverflow.com/questions/1371307/displayblock-inside-displayinline
        //
        if (!BLOCK_RENDERING(rend_flags, BOX_INLINE_BLOCKS)) {
            // No support for anything but inline elements, and possibly embedded floats
            recurseMatchingElements(resetRendMethodToInline, isNotBoxWrappingNode);
        } else if (!isNotBoxWrappingNode(this)) {
            // If this node is already a box wrapping node (active floatBox or inlineBox,
            // possibly a <inlineBox T="EmbeddedBlock"> created here in a previous
            // rendering), just set it to erm_inline.
            setRendMethod(erm_inline);
        } else {
            // Set this inline element to be erm_inline, and look at its children
            setRendMethod(erm_inline);
            // Quick scan first, before going into more checks if needed
            bool has_block_nodes = false;
            bool has_inline_nodes = false;
            for (int i = 0; i < getChildCount(); i++) {
                ldomNode* child = getChildNode(i);
                if (!child->isElement()) // text node
                    continue;
                int cm = child->getRendMethod();
                if (cm == erm_inline) {
                    has_inline_nodes = true; // We won't be able to make it erm_block
                    continue;
                }
                if (cm == erm_invisible || cm == erm_killed)
                    continue;
                if (!isNotBoxWrappingNode(child)) {
                    // This child is already wrapped by a floatBox or inlineBox
                    continue;
                }
                has_block_nodes = true;
                if (has_inline_nodes)
                    break; // we know enough
            }
            if (has_block_nodes) {
                bool has_non_empty_text_nodes = false;
                bool do_wrap_blocks = true;
                if (!has_inline_nodes) {
                    // No real inline nodes. Inspect each text node to see if they
                    // are all empty text.
                    for (int i = 0; i < getChildCount(); i++) {
                        if (getChildNode(i)->isText()) {
                            lString32 s = getChildNode(i)->getText();
                            if (!IsEmptySpace(s.c_str(), s.length())) {
                                has_non_empty_text_nodes = true;
                                break;
                            }
                        }
                    }
                    if (!has_non_empty_text_nodes) {
                        // We can be a block wrapper (renderBlockElementEnhanced/Legacy will
                        // skip empty text nodes, no need to remove them)
                        setRendMethod(erm_block);
                        do_wrap_blocks = false;
                    }
                }
                if (do_wrap_blocks) {
                    // We have a mix of inline nodes or non-empty text, and block elements:
                    // wrap each block element in a <inlineBox T="EmbeddedBlock">.
                    for (int i = getChildCount() - 1; i >= 0; i--) {
                        ldomNode* child = getChildNode(i);
                        if (!child->isElement()) // text node
                            continue;
                        int cm = child->getRendMethod();
                        if (cm == erm_inline || cm == erm_invisible || cm == erm_killed)
                            continue;
                        if (!isNotBoxWrappingNode(child))
                            continue;
                        // This child is erm_block or erm_final (or some other erm_table like rend method).
                        // It will be inside a upper erm_final
                        // Wrap this element into an inlineBox, just as if it was display:inline-block,
                        // with a few differences that will be handled by lvrend.cpp/lvtextfm.cpp:
                        // - it should behave like if it has width: 100%, so preceeding
                        //   and following text/inlines element will be on their own line
                        // - the previous line should not be justified
                        // - in the matter of page splitting, lines (as they are 100%-width) should
                        //   be forwarded to the parent flow/context
                        // Remove any preceeding or following empty text nodes (there can't
                        // be consecutive text nodes) so we don't get spurious empty lines.
                        if (i < getChildCount() - 1 && getChildNode(i + 1)->isText()) {
                            lString32 s = getChildNode(i + 1)->getText();
                            if (IsEmptySpace(s.c_str(), s.length())) {
                                removeChildren(i + 1, i + 1);
                            }
                        }
                        if (i > 0 && getChildNode(i - 1)->isText()) {
                            lString32 s = getChildNode(i - 1)->getText();
                            if (IsEmptySpace(s.c_str(), s.length())) {
                                removeChildren(i - 1, i - 1);
                                i--; // update our position
                            }
                        }
                        ldomNode* ibox = insertChildElement(i, LXML_NS_NONE, el_inlineBox);
                        moveItemsTo(ibox, i + 1, i + 1); // move this child from 'this' into ibox
                        // Mark this inlineBox so we can handle its pecularities
                        ibox->setAttributeValue(LXML_NS_NONE, attr_T, U"EmbeddedBlock");
                        setNodeStyle(ibox, getStyle(), getFont());
                        ibox->setRendMethod(erm_inline);
                    }
                }
            }
        }
    } else if (d == css_d_ruby) {
        // This will be dealt in a big section below. For now, reset everything
        // to inline as ruby is only allowed to contain inline content.
        // We don't support the newer display: values like ruby-base, ruby-text...,
        // but only "display: ruby" which is just set on the <ruby> element
        // (which allows us to have it reset back to "display: inline" if we
        // don't wan't ruby support).
        //   recurseElements( resetRendMethodToInline );
        // Or may be not: looks like we can support <ruby> inside <ruby>,
        // so allow that; and probably anything nested, as we'll handle
        // that just like a table cell content.
        setRendMethod(erm_inline);
    } else if (d == css_d_run_in) {
        // runin
        //CRLog::trace("switch all children elements of <%s> to inline", LCSTR(getNodeName()));
        recurseElements(resetRendMethodToInline);
        setRendMethod(erm_inline);
    } else if (d == css_d_list_item_legacy) {
        // list item (no more used, obsolete rendering method)
        setRendMethod(erm_final);
    } else if (d == css_d_table) {
        // table: this will "Generate missing child wrappers" if needed
        initTableRendMethods(this, 0);
        // Not sure if we should do the same for the other css_d_table_* and
        // call initTableRendMethods(this, 1/2/3) so that the "Generate missing
        // child wrappers" step is done before the "Generate missing parents" step
        // we might be doing below - to conform to the order of steps in the specs.
    } else if (d == css_d_inline_table && (BLOCK_RENDERING(rend_flags, COMPLETE_INCOMPLETE_TABLES) || getNodeId() == el_table)) {
        // Only if we're able to complete incomplete tables, or if this
        // node is itself a <TABLE>. Otherwise, fallback to the following
        // catch-all 'else' and render its content as block.
        //   (Note that we should skip that if the node is an image, as
        //   initTableRendMethods() would not be able to do anything with
        //   it as it can't add children to an IMG. Hopefully, the specs
        //   say replaced elements like IMG should not have table-like
        //   display: values - which setNodeStyle() ensures.)
        // Any element can have "display: inline-table", and if it's not
        // a TABLE, initTableRendMethods() will complete/wrap it to make
        // it possibly the single cell of a TABLE. This should naturally
        // ensure all the differences between inline-block and inline-table.
        // https://stackoverflow.com/questions/19352072/what-is-the-difference-between-inline-block-and-inline-table/19352149#19352149
        initTableRendMethods(this, 0);
        // Note: if (d==css_d_inline_block && getNodeId()==el_table), we
        // should NOT call initTableRendMethods()! It should be rendered
        // as a block, and if its children are actually TRs, they will be
        // wrapped in a "missing parent" tabularBox wrapper that will
        // have initTableRendMethods() called on it.
    } else {
        // block or final
        // remove last empty space text nodes
        bool hasBlockItems = false;
        bool hasInline = false;
        bool hasFloating = false;
        // Floating nodes, thus block, are accounted apart from inlines
        // and blocks, as their behaviour is quite specific.
        // - When !PREPARE_FLOATBOXES, we just don't deal specifically with
        //   floats, for a rendering more similar to legacy rendering: SPANs
        //   with float: will be considered as non-floating inline, while
        //   DIVs with float: will be considered as block elements, possibly
        //   causing autoBoxing of surrounding content with only inlines.
        // - When PREPARE_FLOATBOXES (even if !FLOAT_FLOATBOXES), we do prepare
        //   floats and floatBoxes to be consistent, ready to be floating, or
        //   not and flat (with a rendering possibly not similar to legacy),
        //   without any display hash mismatch (so that toggling does not
        //   require a full reloading). SPANs and DIVs with float: mixed with
        //   inlines will be considered as inline when !FLOAT_FLOATBOXES, to
        //   avoid having autoBoxing elements that would mess with a correct
        //   floating rendering.
        // Note that FLOAT_FLOATBOXES requires having PREPARE_FLOATBOXES.
        bool handleFloating = BLOCK_RENDERING(rend_flags, PREPARE_FLOATBOXES);

        detectChildTypes(this, hasBlockItems, hasInline, hasInternalTableItems, hasFloating, handleFloating);
        const css_elem_def_props_t* ntype = getElementTypePtr();
        if (ntype && ntype->is_object) { // image
            // No reason to erm_invisible an image !
            // And it has to be erm_final to be drawn (or set to erm_inline
            // by some upper node).
            // (Note that setNodeStyle() made sure an image can't be
            // css_d_inline_table/css_d_table*, as per specs.)
            setRendMethod(erm_final);
            /* used to be:
            switch ( d )
            {
            case css_d_block:
            case css_d_list_item_block:
            case css_d_inline:
            case css_d_inline_block:
            case css_d_inline_table:
            case css_d_run_in:
                setRendMethod( erm_final );
                break;
            default:
                //setRendMethod( erm_invisible );
                recurseElements( resetRendMethodToInvisible );
                break;
            }
            */
        } else if (hasBlockItems && !hasInline) {
            // only blocks (or floating blocks) inside
            setRendMethod(erm_block);
        } else if (!hasBlockItems && hasInline) {
            // only inline (with possibly floating blocks that will
            // be dealt with by renderFinalBlock)
            if (hasFloating) {
                // If all the inline elements are empty space, we may as well
                // remove them and have our floats contained in a erm_block
                if (cleanIfOnlyEmptyTextInline(true)) {
                    setRendMethod(erm_block);
                } else {
                    if (!BLOCK_RENDERING(rend_flags, FLOAT_FLOATBOXES)) {
                        // If we don't want floatBoxes floating, reset them to be
                        // rendered inline among inlines
                        recurseMatchingElements(resetRendMethodToInline, isNotBoxingInlineBoxNode);
                    }
                    setRendMethod(erm_final);
                }
            } else {
                setRendMethod(erm_final);
            }
        } else if (!hasBlockItems && !hasInline) {
            // nothing (or only floating blocks)
            // (don't ignore it as it might be some HR with borders/padding,
            // even if no content)
            setRendMethod(erm_block);
        } else if (hasBlockItems && hasInline) {
            // Mixed content of blocks and inline elements:
            // the consecutive inline elements should be considered part
            // of an anonymous block element - non-anonymous for crengine,
            // as we create a <autoBoxing> element and add it to the DOM),
            // taking care of ignoring unvaluable inline elements consisting
            // of only spaces.
            //   Note: when there are blocks, inlines and floats mixed, we could
            //   choose to let the floats be blocks, or include them with the
            //   surrounding inlines into an autoBoxing:
            //   - blocks: they will just be footprints (so, only 2 squares at
            //   top left and right) over the inline/final content, and when
            //   there are many, the text may not wrap fully around the floats...
            //   - with inlines: they will wrap fully, but if the text is short,
            //   the floats will be cleared, and there will be blank vertical
            //   filling space...
            //   The rendering can be really different, and there's no real way
            //   of knowing which will be the best.
            //   So, for now, go with including them with inlines into the
            //   erm_final autoBoxing.
            // The above has become less critical after we added DO_NOT_CLEAR_OWN_FLOATS
            // and ALLOW_EXACT_FLOATS_FOOTPRINTS, and both options should render
            // similarly.
            // But still going with including them with inlines is best, as we
            // don't need to include them in the footprint (so, the limit of
            // 5 outer block float IDs is still available for real outer floats).
            if (getParentNode()->getNodeId() == el_autoBoxing) {
                // already autoboxed
                setRendMethod(erm_final);
                // This looks wrong: no reason to force child of autoBoxing to be
                // erm_final: most often, the autoBoxing has been created to contain
                // only inlines and set itself to be erm_final. So, it would have been
                // caught by the 'else if ( !hasBlockItems && hasInline )' above and
                // set to erm_final. If not, styles have changed, and it may contain
                // a mess of styles: it might be better to proceed with the following
                // cleanup (and have autoBoxing re-autoboxed... or not at all when
                // a cache file is used, and we'll end up being erm_final anyway).
                // But let's keep it, in case it handles some edge cases.
            } else {
                // cleanup or autobox
                int i = getChildCount() - 1;
                for (; i >= 0; i--) {
                    ldomNode* node = getChildNode(i);

                    // DEBUG TEST
                    // if ( getParentNode()->getChildIndex( getDataIndex() )<0 ) {
                    //    CRLog::error("Invalid parent->child relation for nodes %d->%d",
                    //              getParentNode()->getDataIndex(), getDataIndex() );
                    // }

                    // We want to keep float:'ing nodes with inline nodes, so they stick with their
                    // siblings inline nodes in an autoBox: the erm_final autoBox will deal
                    // with rendering the floating node, and the inline text around it
                    if (isInlineNode(node) || (handleFloating && isFloatingNode(node))) {
                        int j = i - 1;
                        for (; j >= 0; j--) {
                            node = getChildNode(j);
                            if (!isInlineNode(node) && !(handleFloating && isFloatingNode(node)))
                                break;
                        }
                        j++;
                        // j..i are inline
                        if (j > 0 || i < (int)getChildCount() - 1)
                            autoboxChildren(j, i, handleFloating);
                        i = j;
                    } else if (i > 0 && node->getRendMethod() == erm_final) {
                        // (We skip the following if the current node is not erm_final, as
                        // if it is erm_block, we would break the block layout by making
                        // it all inline in an erm_final autoBoxing.)
                        // This node is not inline, but might be preceeded by a css_d_run_in node:
                        // https://css-tricks.com/run-in/
                        // https://developer.mozilla.org/en-US/docs/Web/CSS/display
                        //   "If the adjacent sibling of the element defined as "display: run-in" box
                        //   is a block box, the run-in box becomes the first inline box of the block
                        //   box that follows it. "
                        // Hopefully only used for footnotes in fb2 where the footnote number
                        // is in a block element, and the footnote text in another.
                        // fb2.css sets the first block to be "display: run-in" as an
                        // attempt to render both on the same line:
                        //   <section id="n1">
                        //     <title style="display: run-in; font-weight: bold;">
                        //       <p>1</p>
                        //     </title>
                        //     <p>Text footnote</p>
                        //   </section>
                        //
                        // This node might be that second block: look if preceeding node
                        // is "run-in", and if it is, bring them both in an autoBoxing.
                        ldomNode* prev = getChildNode(i - 1);
                        ldomNode* inBetweenTextNode = NULL;
                        if (prev->isText() && i - 1 > 0) { // some possible empty text in between
                            inBetweenTextNode = prev;
                            prev = getChildNode(i - 2);
                        }
                        if (prev->isElement() && prev->getStyle()->display == css_d_run_in) {
                            bool do_autoboxing = true;
                            int run_in_idx = inBetweenTextNode ? i - 2 : i - 1;
                            int block_idx = i;
                            if (inBetweenTextNode) {
                                lString32 text = inBetweenTextNode->getText();
                                if (IsEmptySpace(text.c_str(), text.length())) {
                                    removeChildren(i - 1, i - 1);
                                    block_idx = i - 1;
                                } else {
                                    do_autoboxing = false;
                                }
                            }
                            if (do_autoboxing) {
                                CRLog::debug("Autoboxing run-in items");
                                // Sadly, to avoid having an erm_final inside another erm_final,
                                // we need to reset the block node to be inline (but that second
                                // erm_final would have been handled as inline anyway, except
                                // for possibly updating the strut height/baseline).
                                node->recurseMatchingElements(resetRendMethodToInline, isNotBoxingInlineBoxNode);
                                // No need to autobox if there are only 2 children (the run-in and this box)
                                if (getChildCount() != 2) { // autobox run-in
                                    autoboxChildren(run_in_idx, block_idx, handleFloating);
                                }
                            }
                            i = run_in_idx;
                        }
                    }
                }
                // check types after autobox
                detectChildTypes(this, hasBlockItems, hasInline, hasInternalTableItems, hasFloating, handleFloating);
                if (hasInline) {
                    // Should not happen when autoboxing has been done above - but
                    // if we couldn't, fallback to erm_final that will render all
                    // children as inline
                    setRendMethod(erm_final);
                } else {
                    // All inlines have been wrapped into block autoBoxing elements
                    // (themselves erm_final): we can be erm_block
                    setRendMethod(erm_block);
                }
            }
        }
    }

    if (hasInternalTableItems && BLOCK_RENDERING(rend_flags, COMPLETE_INCOMPLETE_TABLES) && getRendMethod() == erm_block) {
        // We have only block items, whether the original ones or the
        // autoBoxing nodes we created to wrap inlines, and all empty
        // inlines have been removed.
        // Some of these block items are css_d_table_cell, css_d_table_row...:
        // if this node (their parent) has not the expected css_d_table_row
        // or css_d_table display style, we are an unproper parent: we want
        // to add the missing parent(s) as wrapper(s) between this node and
        // these children.
        // (If we ended up not being erm_block, and we contain css_d_table_*
        // elements, everything is already messed up.)
        // Note: we first used the same <autoBoxing> element used to box
        // inlines as the table wrapper, which was fine, except in some edge
        // cases where some real autoBoxing were wrongly re-used as the tabular
        // wrapper (and we ended up having erm_final containing other erm_final
        // which were handled just like erm_inline with ugly side effects...)
        // So, best to introduce a decicated element: <tabularBox>.
        //
        // We follow rules from section "Generate missing parents" in:
        //   https://www.w3.org/TR/CSS22/tables.html#anonymous-boxes
        //   https://www.w3.org/TR/css-tables-3/#fixup (clearer than previous one)
        // Note: we do that not in the order given by the specs... As we walk
        // nodes deep first, we are here first "generating missing parents".
        // When walking up, and meeting a real css_d_table element, or
        // below when adding a generated erm_table tabularBox, we call
        // initTableRendMethods(0), which will "generate missing child wrappers".
        // Not really sure both orderings are equivalent, but let's hope it's ok...

        // So, let's generate missing parents:

        // "An anonymous table-row box must be generated around each sequence
        // of consecutive table-cell boxes whose parent is not a table-row."
        if (d != css_d_table_row) { // We're not a table row
            // Look if we have css_d_table_cell that we must wrap in a proper erm_table_row
            int last_table_cell = -1;
            int first_table_cell = -1;
            int last_visible_child = -1;
            bool did_wrap = false;
            int len = getChildCount();
            for (int i = len - 1; i >= 0; i--) {
                ldomNode* child = getChildNode(i);
                int cd = child->getStyle()->display;
                int cm = child->getRendMethod();
                if (cd == css_d_table_cell) {
                    if (last_table_cell < 0) {
                        last_table_cell = i;
                        // We've met a css_d_table_cell, see if it is followed by
                        // tabularBox siblings we might have passed by: they might
                        // have been added by initTableRendMethods as a missing
                        // children of a css_d_table_row: make them part of the row.
                        for (int j = i + 1; j < getChildCount(); j++) {
                            if (getChildNode(j)->getNodeId() == el_tabularBox)
                                last_table_cell = j;
                            else
                                break;
                        }
                    }
                    if (i == 0)
                        first_table_cell = 0;
                    if (last_visible_child < 0)
                        last_visible_child = i;
                } else if (last_table_cell >= 0 && child->getNodeId() == el_tabularBox) {
                    // We've seen a css_d_table_cell and we're seeing a tabularBox:
                    // it might have been added by initTableRendMethods as a missing
                    // children of a css_d_table_row: make it part of the row
                    if (i == 0)
                        first_table_cell = 0;
                    if (last_visible_child < 0)
                        last_visible_child = i;
                } else if (cd == css_d_none || cm == erm_invisible) {
                    // Can be left inside or outside the wrap
                    if (i == 0 && last_table_cell >= 0) {
                        // Include it if first and we're wrapping
                        first_table_cell = 0;
                    }
                } else {
                    if (last_table_cell >= 0)
                        first_table_cell = i + 1;
                    if (last_visible_child < 0)
                        last_visible_child = i;
                }
                if (first_table_cell >= 0) {
                    if (first_table_cell == 0 && last_table_cell == last_visible_child && getNodeId() == el_tabularBox && !did_wrap) {
// All children are table cells, and we're not css_d_table_row,
// but we are a tabularBox!
// We were most probably created here in a previous rendering,
// so just set us to be the anonymous table row.
#ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                        printf("initNodeRendMethod: (reused)wrapping unproper table cells %d>%d\n",
                               first_table_cell, last_table_cell);
#endif
                        setRendMethod(erm_table_row);
                    } else {
#ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                        printf("initNodeRendMethod: wrapping unproper table cells %d>%d\n",
                               first_table_cell, last_table_cell);
#endif
                        ldomNode* tbox = boxWrapChildren(first_table_cell, last_table_cell, el_tabularBox);
                        if (tbox && !tbox->isNull()) {
                            tbox->initNodeStyle();
                            tbox->setRendMethod(erm_table_row);
                        }
                        did_wrap = true;
                    }
                    last_table_cell = -1;
                    first_table_cell = -1;
                }
            }
        }

        // "An anonymous table or inline-table box must be generated around each
        // sequence of consecutive proper table child boxes which are misparented."
        // Not sure if we should skip that for some values of this node's
        // style->display among css_d_table*. Let's do as litterally as the specs.
        int last_misparented = -1;
        int first_misparented = -1;
        int last_visible_child = -1;
        bool did_wrap = false;
        int len = getChildCount();
        for (int i = len - 1; i >= 0; i--) {
            ldomNode* child = getChildNode(i);
            int cd = child->getStyle()->display;
            int cm = child->getRendMethod();
            bool is_misparented = false;
            if ((cd == css_d_table_row || cm == erm_table_row) && d != css_d_table && d != css_d_table_row_group && d != css_d_table_header_group && d != css_d_table_footer_group) {
                // A table-row is misparented if its parent is neither a table-row-group
                // nor a table-root box (we include by checking cm==erm_table_row any
                // anonymous table row created just above).
                is_misparented = true;
            } else if (cd == css_d_table_column && d != css_d_table && d != css_d_table_column_group) {
                // A table-column box is misparented if its parent is neither
                // a table-column-group box nor a table-root box.
                is_misparented = true;
            } else if (d != css_d_table && (cd == css_d_table_row_group || cd == css_d_table_header_group || cd == css_d_table_footer_group || cd == css_d_table_column_group || cd == css_d_table_caption)) {
                // A table-row-group, table-column-group, or table-caption box is misparented
                // if its parent is not a table-root box.
                is_misparented = true;
            }
            if (is_misparented) {
                if (last_misparented < 0) {
                    last_misparented = i;
                    // As above for table cells: grab passed-by tabularBox siblings
                    // to include them in the wrap
                    for (int j = i + 1; j < getChildCount(); j++) {
                        if (getChildNode(j)->getNodeId() == el_tabularBox)
                            last_misparented = j;
                        else
                            break;
                    }
                }
                if (i == 0)
                    first_misparented = 0;
                if (last_visible_child < 0)
                    last_visible_child = i;
            } else if (last_misparented >= 0 && child->getNodeId() == el_tabularBox) {
                // As above for table cells: include tabularBox siblings in the wrap
                if (i == 0)
                    first_misparented = 0;
                if (last_visible_child < 0)
                    last_visible_child = i;
            } else if (cd == css_d_none || cm == erm_invisible) {
                // Can be left inside or outside the wrap
                if (i == 0 && last_misparented >= 0) {
                    // Include it if first and we're wrapping
                    first_misparented = 0;
                }
            } else {
                if (last_misparented >= 0)
                    first_misparented = i + 1;
                if (last_visible_child < 0)
                    last_visible_child = i;
            }
            if (first_misparented >= 0) {
                if (first_misparented == 0 && last_misparented == last_visible_child && getNodeId() == el_tabularBox && !did_wrap) {
// All children are misparented, and we're not css_d_table,
// but we are a tabularBox!
// We were most probably created here in a previous rendering,
// so just set us to be the anonymous table.
#ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                    printf("initNodeRendMethod: (reused)wrapping unproper table children %d>%d\n",
                           first_misparented, last_misparented);
#endif
                    setRendMethod(erm_table);
                    initTableRendMethods(this, 0);
                } else {
#ifdef DEBUG_INCOMPLETE_TABLE_COMPLETION
                    printf("initNodeRendMethod: wrapping unproper table children %d>%d\n",
                           first_misparented, last_misparented);
#endif
                    ldomNode* tbox = boxWrapChildren(first_misparented, last_misparented, el_tabularBox);
                    if (tbox && !tbox->isNull()) {
                        tbox->initNodeStyle();
                        tbox->setRendMethod(erm_table);
                        initTableRendMethods(tbox, 0);
                    }
                    did_wrap = true;
                }
                last_misparented = -1;
                first_misparented = -1;
                // Note:
                //   https://www.w3.org/TR/css-tables-3/#fixup
                //   "An anonymous table or inline-table box must be generated
                //    around [...] If the box's parent is an inline, run-in, or
                //    ruby box (or any box that would perform inlinification of
                //    its children), then an inline-table box must be generated;
                //    otherwise it must be a table box."
                // We don't handle the "inline parent > inline-table" rule,
                // because of one of the first checks at top of this function:
                // if this node (the parent) is css_d_inline, we didn't have
                // any detectChildTypes() and autoBoxing happening, stayed erm_inline
                // and didn't enter this section to do the tabularBox wrapping.
                // Changing this (incorrect) rule for css_d_inline opens many
                // bigger issues, so let's not support this (rare) case here.
                // So:
                //   <div>Some text <span style="display: table-cell">table-cell</span> and more text.</div>
                // will properly have the cell tabularBoxes'ed, which will be
                // inserted between 2 autoBoxing (the text nodes), because their
                // container is css_d_block DIV.
                // While:
                //   <div><span>Some text <span style="display: table-cell">table-cell</span> and more text.</span></div>
                // as the container is a css_d_inline SPAN, nothing will happen
                // and everything will be reset to erm_inline. The parent DIV
                // will just see that it contains a single erm_inline SPAN,
                // and won't do any boxing.
            }
        }
    }

    if (d == css_d_ruby && BLOCK_RENDERING(rend_flags, ENHANCED)) {
        // Ruby input can be quite loose and have various tag strategies (mono/group,
        // interleaved/tabular, double sided). Moreover, the specs have evolved between
        // 2001 and 2020 (<rbc> tag no more mentioned in 2020; <rtc> being just another
        // semantic container for Mozilla, and can be preceded by a bunch of <rt> which
        // are pronunciation containers, that don't have to be in an <rtc>...)
        // Moreover, various samples on the following pages don't close tags, and expect
        // the HTML parser to do that. We do that only when parsing .html files, but
        // we don't when parsing .epub files as they are expected to be balanced XHTML.
        //
        // References:
        //  https://www.w3.org/International/articles/ruby/markup
        //  https://www.w3.org/TR/ruby-use-cases/ differences between XHTML, HTML5 & HTML Extensions
        //  https://www.w3.org/TR/ruby/ Ruby Annotation, 2001
        //  http://darobin.github.io/html-ruby/ HTML Ruby Markup Extensions, 2015
        //  https://html.spec.whatwg.org/multipage/text-level-semantics.html#the-ruby-element HTML Living standard
        //  https://drafts.csswg.org/css-ruby/ CSS Ruby Layout, 2020
        //  https://developer.mozilla.org/en-US/docs/Web/HTML/Element/rtc
        //  https://chenhuijing.com/blog/html-ruby/ All about the HTML <ruby> element (in 2016)
        //  https://github.com/w3c/html/issues/291 How to handle legacy Ruby content that may use <rbc>?
        //  https://w3c.github.io/i18n-tests/results/ruby-html Browsers support
        //
        // We can handle quite a few of these variations with the following strategy.
        //
        // We want a <ruby> (which will stay inline) to only contain inlineBox>rubyBox elements
        // that will be set up to be rendered just as an inline-table:
        //   <ruby, "display: ruby", erm_inline>
        //     <inlineBox, erm_inline>  [1 or more, 1 per ruby segment]
        //       <rubyBox, erm_table>   [1]
        //         <rbc or rubyBox, erm_table_row>  [1]
        //           <rb or rubyBox, erm_final> base text </rb or /rubyBox>  [1 or more]
        //         </rbc or /rubyBox>
        //         <rtc or rubyBox, erm_table_row>  [1 or more, usually 1 or 2]
        //           <rt or rubyBox, erm_final> annotation text </rt or /rubyBox>  [1 or more]
        //         </rtc or /rubyBox>
        //       </rubyBox>
        //     </inlineBox>
        //     [some possible empty space text nodes between ruby segments]
        //   </ruby>
        //
        // (The re-ordering of the table rows, putting the first "rtc" above the "rbc",
        // will be done in renderTable(), as it is just needed there in its own internal
        // table data structures. The DOM will stay in its original order: the "rbc"
        // staying before followup "rtc", which will give us the correct baseline to use
        // for the whole structure: the baseline of the "rbc".
        //
        // We need to build all this when we meet a simple:
        //   <ruby>text1<rt>annot1</rt>text2<rt>annot2</rt> </ruby>
        // The only element we'll nearly always find inside a <ruby> is <rt>,
        // (but we can find sometimes a single <rtc> with no <rt>).
        //
        // One thing we might not handle well is white-space, which, depending on where
        // it happens, should be dropped or not. We drop some by putting it between table
        // elements, we keep some by putting it between the inlineBoxes, but not really
        // according to the complex rules in https://drafts.csswg.org/css-ruby/#box-fixup
        //
        // Some other notes:
        // - We can style some ruby elements, including some of the rubyBox we add, with:
        //     rt, rubyBox[T=rt] { font-size: 50%; font-variant-east-asian: ruby; }
        //     rubyBox { border: 1px solid green; }
        // - Note that on initial loading (HTML parsing, and this boxing here happening,
        //   the real ruby sub-elements present in the HTML will already be there in the
        //   DOM and have their style set, possibly inherited from their parent (the <ruby>
        //   element) *before* this boxing is happening. If we add a rubyBox, and it
        //   becomes the parent of a rb or rt, these rb or rt won't inherite from the
        //   rubyBox (that we may style). They also won't get styled by CSS selectors
        //   like "rubyBox > rt".
        //   But on a next re-renderings, as the DOM is kept, all this will happen.
        //   So: avoid such rules, and avoid setting inherit'able properties to
        //   the rubyBox elements; otherwise we may get different look on initial
        //   loading and on subsequent re-renderings.
        // - With some ruby constructs, the behaviour and rendering might be different
        //   whether we're parsing a HTML file or an EPUB file:
        //   - the HTML parser is able to auto-close tags, which is needed with most
        //     of the samples in the above URLs (but may fail on nested ruby with
        //     unbalanced tags, as auto-closing in one ruby might kill the other).
        //   - the EPUB XHTML parser expects balanced tags, and may work with nested
        //     ruby, but will not process ruby with unbalanced tags.

        // To make things easier to follow below (with the amount of nested rubyBoxes...),
        // we name the variables used to hold each of them:
        //   ibox1 : the inlineBox wrapping the 1st level rubyBox that will be erm_table (inline-table)
        //   rbox1 : the 1st level rubyBox that will be erm_table
        //   rbox2 : the 2nd level rubyBox that will be erm_table_row, like existing <rbc> and <rtc>
        //   rbox3 : the 3rd level rubyBox that will be a table cell (erm_final or erm_block), like existing <rb> and <rt>

        // Check if we have already wrapped: we should contain only <inlineBox>'ed <rubyBox>es
        // Note that <ruby style="display: ruby"> is all that is required to trigger this. When
        // wanting to disable ruby support, it's enough to just set <ruby> to "display: inline":
        // a change in "display:" value will cause a nodeDisplayStyleHash mismatch, and propose
        // a full reload with DOM rebuild, which will forget all the rubyBox we added.
        int len = getChildCount();
        bool needs_wrapping = len > 0;
        for (int i = 0; i < len; i++) {
            ldomNode* child = getChildNode(i);
            if (child->isElement() && child->getNodeId() == el_inlineBox && child->getChildCount() > 0 && child->getChildNode(0)->getNodeId() == el_rubyBox) {
                // If we find one <inlineBox><rubyBox>, we created that previously and we ensured
                // there are only rubyBoxes, empty text nodes, or some trailing inline nodes
                // not followed by a <rt>: no need for more checks and work.
                needs_wrapping = false;
                break;
            }
        }
        if (needs_wrapping) {
            // 1) Wrap everything up to (and including consecutive ones) <rt> <rtc> <rp>
            // into <inlineBox><rubyBox>, and continue doing it after that.
            int first_to_wrap = -1;
            int last_to_wrap = -1;
            for (int i = 0; i <= len; i++) {
                ldomNode* child;
                lInt16 elemId;
                bool eoc = i == len; // end of children
                if (!eoc) {
                    child = getChildNode(i);
                    if (child->isElement()) {
                        elemId = child->getNodeId();
                    } else {
                        lString32 s = child->getText();
                        elemId = IsEmptySpace(s.c_str(), s.length()) ? -2 : -1;
                        // When meeting an empty space (elemId==-2), we'll delay wrapping
                        // decision to when we process the next node.
                        // We'll also not start a wrap with it.
                    }
                }
                if (last_to_wrap >= 0 && (eoc || (elemId != el_rt && elemId != el_rtc && elemId != el_rp && elemId != -2))) {
                    if (first_to_wrap < 0)
                        first_to_wrap = 0;
                    ldomNode* rbox1 = boxWrapChildren(first_to_wrap, last_to_wrap, el_rubyBox);
                    if (rbox1 && !rbox1->isNull()) {
                        // Set an attribute for the kind of container we made (Ruby Segment)
                        // so we can style it via CSS.
                        rbox1->setAttributeValue(LXML_NS_NONE, attr_T, U"rseg");
                        rbox1->initNodeStyle();
                        // Update loop index and end
                        int removed = last_to_wrap - first_to_wrap;
                        i = i - removed;
                        len = len - removed;
                        // And wrap this rubyBox in an inlineBox
                        ldomNode* ibox1 = insertChildElement(first_to_wrap, LXML_NS_NONE, el_inlineBox);
                        moveItemsTo(ibox1, first_to_wrap + 1, first_to_wrap + 1);
                        ibox1->initNodeStyle();
                    }
                    first_to_wrap = -1;
                    last_to_wrap = -1;
                }
                if (eoc)
                    break;
                if (elemId == -1) { // isText(), non empty
                    if (first_to_wrap < 0) {
                        first_to_wrap = i;
                    }
                } else if (elemId == -2) { // isText(), empty
                    // Don't start a wrap on it
                } else {
                    if (first_to_wrap < 0) {
                        first_to_wrap = i;
                    }
                    if (elemId == el_rt || elemId == el_rtc || elemId == el_rp) {
                        last_to_wrap = i;
                        // Don't wrap yet: there can be followup other RT/RTC
                    }
                }
            }
            // 2) Enter each rubyBox we have created (they will be inline-table),
            // and wrap its content as needed to make rows (of rubyBox, rbc and rtc)
            // and cells (of rubyBox, rb and rt).
            len = getChildCount();
            for (int i = 0; i < len; i++) {
                ldomNode* ibox1 = getChildNode(i);
                if (!ibox1->isElement() || ibox1->getNodeId() != el_inlineBox)
                    continue;
                ldomNode* rbox1 = ibox1->getChildCount() > 0 ? ibox1->getChildNode(0) : NULL;
                if (!rbox1 || !rbox1->isElement() || rbox1->getNodeId() != el_rubyBox)
                    continue;
                // (Each rbox1 will be set erm_table)
                int len1 = rbox1->getChildCount();
                int first_to_wrap = -1;
                bool ruby_base_wrap_done = false;
                bool ruby_base_present = false;
                for (int i1 = 0; i1 <= len1; i1++) {
                    ldomNode* child;
                    lInt16 elemId;
                    bool eoc = i1 == len1; // end of children
                    if (!eoc) {
                        child = rbox1->getChildNode(i1);
                        if (child->isElement()) {
                            elemId = child->getNodeId();
                        } else {
                            lString32 s = child->getText();
                            elemId = IsEmptySpace(s.c_str(), s.length()) ? -2 : -1;
                            // When meeting an empty space (elemId==-2), we'll delay wrapping
                            // decision to when we process the next node.
                            // We'll also not start a wrap with it.
                        }
                    }
                    if (first_to_wrap >= 0 && (eoc || (!ruby_base_wrap_done && (elemId == el_rtc || elemId == el_rt || elemId == el_rp)) || (ruby_base_wrap_done && elemId == el_rtc))) {
                        ldomNode* rbox2 = rbox1->boxWrapChildren(first_to_wrap, i1 - 1, el_rubyBox);
                        if (rbox2 && !rbox2->isNull()) {
                            // Set an attribute for the kind of container we made (<rbc> or <rtc>-like),
                            // so we can style it like real <rbc> and <rtc> via CSS.
                            rbox2->setAttributeValue(LXML_NS_NONE, attr_T, ruby_base_wrap_done ? U"rtc" : U"rbc");
                            rbox2->initNodeStyle();
                            // Update loop index and end
                            int removed = i1 - 1 - first_to_wrap;
                            i1 = i1 - removed;
                            len1 = len1 - removed;
                        }
                        first_to_wrap = -1;
                        if (!eoc && !ruby_base_wrap_done) {
                            ruby_base_present = true; // We did create it
                        }
                        if (eoc)
                            break;
                    }
                    if (elemId == -1) { // isText(), non empty
                        if (first_to_wrap < 0) {
                            first_to_wrap = i1;
                        }
                    } else if (elemId == -2) { // isText(), empty
                        // Don't start a wrap on it
                    } else {
                        if (elemId == el_rbc || elemId == el_rtc) {
                            // These are fine containers at this level.
                            // (If el_rbc, we shouldn't have found anything before
                            // it; if we did, just ignore it.)
                            first_to_wrap = -1;
                            ruby_base_wrap_done = true;
                            if (elemId == el_rbc)
                                ruby_base_present = true;
                        } else if (first_to_wrap < 0) {
                            first_to_wrap = i1;
                            if (elemId == el_rt || elemId == el_rp) {
                                ruby_base_wrap_done = true;
                            }
                        }
                    }
                }
                if (!ruby_base_present) {
                    // <ruby><rt>annotation</rt></ruby> : add rubyBox for empty base text
                    ldomNode* rbox2 = rbox1->insertChildElement(0, LXML_NS_NONE, el_rubyBox);
                    rbox2->setAttributeValue(LXML_NS_NONE, attr_T, U"rbc");
                    rbox2->initNodeStyle();
                }
                // rbox1 now contains only <rbc>, <rtc> or <rubyBox> (which will be set erm_table_row)
                // 3) for each, ensure its content is <rb>, <rt>, and if not, wrap it in
                // a <rubyBox> (these will be all like table cells, set erm_final)
                len1 = rbox1->getChildCount();
                bool ruby_base_seen = false;
                for (int i1 = 0; i1 < len1; i1++) {
                    ldomNode* rbox2 = rbox1->getChildNode(i1);
                    if (!rbox2->isElement())
                        continue;
                    lInt16 elemId = rbox2->getNodeId();
                    lInt16 expected_child_elem_id;
                    if (elemId == el_rbc) {
                        expected_child_elem_id = el_rb;
                    } else if (elemId == el_rtc) {
                        expected_child_elem_id = el_rt;
                    } else if (elemId == el_rubyBox) {
                        expected_child_elem_id = ruby_base_seen ? el_rt : el_rb;
                    } else { // unexpected
                        continue;
                    }
                    ruby_base_seen = true; // We're passing by a container, the first one being the base
                    bool has_expected = false;
                    int len2 = rbox2->getChildCount();
                    for (int i2 = 0; i2 < len2; i2++) {
                        ldomNode* child = rbox2->getChildNode(i2);
                        lInt16 childElemId = child->isElement() ? child->getNodeId() : -1;
                        if (childElemId == expected_child_elem_id) {
                            // If a single expected is found, assume everything is fine
                            // (other badly wrapped elements will just be ignored and invisible)
                            has_expected = true;
                            break;
                        }
                    }
                    if (!has_expected) {
                        // Wrap everything into a rubyBox
                        if (len2 > 0) { // some children to wrap
                            ldomNode* rbox3 = rbox2->boxWrapChildren(0, len2 - 1, el_rubyBox);
                            if (rbox3 && !rbox3->isNull()) {
                                rbox3->setAttributeValue(LXML_NS_NONE, attr_T, expected_child_elem_id == el_rb ? U"rb" : U"rt");
                                if (elemId == el_rtc) {
                                    // Firefox makes a <rtc>text</rtc> (without any <rt>) span the whole involved base
                                    rbox3->setAttributeValue(LXML_NS_NONE, attr_rbspan, U"99"); // (our max supported)
                                }
                                rbox3->initNodeStyle();
                            }
                        } else { // no child to wrap
                            // We need to insert an empty element to play the role of a <td> for
                            // the table rendering code to work correctly.
                            ldomNode* rbox3 = rbox2->insertChildElement(0, LXML_NS_NONE, el_rubyBox);
                            rbox3->setAttributeValue(LXML_NS_NONE, attr_T, expected_child_elem_id == el_rb ? U"rb" : U"rt");
                            rbox3->initNodeStyle();
                            // We need to add some text for the cell to ensure its height.
                            // We add a ZERO WIDTH SPACE, which will not collapse into nothing
                            rbox3->insertChildText(U"\x200B");
                        }
                    }
                }
            }
        }
        // All wrapping done, or assumed to have already been done correctly.
        // We can set the rendering methods to make all this a table.
        // All unexpected elements will be erm_invisible
        len = getChildCount();
        for (int i = 0; i < len; i++) {
            ldomNode* ibox1 = getChildNode(i);
            if (!ibox1->isElement() || ibox1->getNodeId() != el_inlineBox)
                continue;
            ibox1->setRendMethod(erm_inline);
            ldomNode* rbox1 = ibox1->getChildCount() > 0 ? ibox1->getChildNode(0) : NULL;
            if (rbox1 && rbox1->isElement() && rbox1->getNodeId() == el_rubyBox) {
                // First level rubyBox: each will be an inline table
                rbox1->setRendMethod(erm_table);
                int len1 = rbox1->getChildCount();
                for (int i1 = 0; i1 < len1; i1++) {
                    ldomNode* rbox2 = rbox1->getChildNode(i1);
                    if (rbox2->isElement()) {
                        rbox2->setRendMethod(erm_invisible);
                        lInt16 rb2elemId = rbox2->getNodeId();
                        if (rb2elemId == el_rubyBox || rb2elemId == el_rbc || rb2elemId == el_rtc) {
                            // Second level rubyBox: each will be a table row
                            rbox2->setRendMethod(erm_table_row);
                            int len2 = rbox2->getChildCount();
                            for (int i2 = 0; i2 < len2; i2++) {
                                ldomNode* rbox3 = rbox2->getChildNode(i2);
                                if (rbox3->isElement()) {
                                    rbox3->setRendMethod(erm_invisible);
                                    lInt16 rb3elemId = rbox3->getNodeId();
                                    if (rb3elemId == el_rubyBox || rb3elemId == el_rb || rb3elemId == el_rt) {
                                        // Third level rubyBox: each will be a table cell.
                                        // (As all it content has previously been reset to erm_inline)
                                        //  /\ This is no more true, but we expect to find inline
                                        //  content, with possibly some nested ruby.
                                        // We can have the cell erm_final.
                                        rbox3->setRendMethod(erm_final);
                                    }
                                    // We let <rp> be invisible like other unexpected elements
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    bool handled_as_float = false;
    if (BLOCK_RENDERING(rend_flags, WRAP_FLOATS)) {
        // While loading the document, we want to put any element with float:left/right
        // inside an internal floatBox element with no margin in its style: this
        // floatBox's RenderRectAccessor will have the position and width/height
        // of the outer element (with margins inside), while the RenderRectAccessor
        // of the wrapped original element itself will have the w/h of the element,
        // including borders but excluding margins (as it is done for all elements
        // by crengine).
        // That makes out the following rules:
        // - a floatBox has a single child: the original floating element.
        // - a non-floatBox element with style->float_ must be wrapped in a floatBox
        //   which will get the same style->float_ (happens in the initial document
        //   loading)
        // - if it already has a floatBox parent, no need to do it again, just ensure
        //   the style->float_ are the same (happens when re-rendering)
        // - if the element has lost its style->float_ (style tweak applied), or
        //   WRAP_FLOATS disabled, as we can't remove the floatBox (we can't
        //   modify the DOM once a cache has been made): update the floatBox's
        //   style->float_ and style->display and rendering method to be the same
        //   as the element: this will limit the display degradation when such
        //   change happen (but a full re-loading will still be suggested to the
        //   user, and should probably be accepted).
        // So, to allow toggling FLOAT_FLOATBOXES with less chance of getting
        // a _nodeDisplayStyleHash change (and so, a need for document reloading),
        // it's best to use WRAP_FLOATS even when flat rendering is requested.
        //
        // Note that, when called in the XML loading phase, we can't update
        // a node style (with getStyle(), copystyle(), setStyle()) as, for some reason
        // not pinpointed, it could affect and mess with the upcoming node parsing.
        // We can just set the style of an element we add (and only once, setting it
        // twice would cause the same mess). But in the re-rendering phase, we can
        // update a node style as much as we want.
        bool isFloating = getStyle()->float_ > css_f_none;
        bool isFloatBox = (getNodeId() == el_floatBox);
        if (isFloating || isFloatBox) {
            handled_as_float = true;
            ldomNode* parent = getParentNode();
            bool isFloatBoxChild = (parent && (parent->getNodeId() == el_floatBox));
            if (isFloatBox) {
                // Wrapping floatBox already made
                if (getChildCount() != 1) {
                    CRLog::error("floatBox with zero or more than one child");
                    crFatalError();
                }
                // Update floatBox style according to child's one
                ldomNode* child = getChildNode(0);
                css_style_ref_t child_style = child->getStyle();
                css_style_ref_t my_style = getStyle();
                css_style_ref_t my_new_style(new css_style_rec_t);
                copystyle(my_style, my_new_style);
                my_new_style->float_ = child_style->float_;
                if (child_style->display <= css_d_inline) { // when !PREPARE_FLOATBOXES
                    my_new_style->display = css_d_inline;   // become an inline wrapper
                } else if (child_style->display == css_d_none) {
                    my_new_style->display = css_d_none; // stay invisible
                } else {                                // everything else (including tables) must be wrapped by a block
                    my_new_style->display = css_d_block;
                }
                setStyle(my_new_style);
                // When re-rendering, setNodeStyle() has already been called to set
                // our style and font, so no need for calling initNodeFont() here,
                // as we didn't change anything related to font in the style (and
                // calling it can cause a style hash mismatch for some reason).

                // Update floatBox rendering method according to child's one
                // It should be erm_block by default (the child can be erm_final
                // if it contains text), except if the child has stayed inline
                // when !PREPARE_FLOATBOXES
                if (child->getRendMethod() == erm_inline)
                    setRendMethod(erm_inline);
                else if (child->getRendMethod() == erm_invisible)
                    setRendMethod(erm_invisible);
                else
                    setRendMethod(erm_block);
            } else if (isFloatBoxChild) {
                // Already floatBox'ed, nothing special to do
            } else if (parent) { // !isFloatBox && !isFloatBoxChild
                // Element with float:, that has not been yet wrapped in a floatBox.
                // Replace this element with a floatBox in its parent children collection,
                // and move it inside, as the single child of this floatBox.
                int pos = getNodeIndex();
                ldomNode* fbox = parent->insertChildElement(pos, LXML_NS_NONE, el_floatBox);
                parent->moveItemsTo(fbox, pos + 1, pos + 1); // move this element from parent into fbox

                // If we have float:, this just-created floatBox should be erm_block,
                // unless the child has been kept inline
                if (!BLOCK_RENDERING(rend_flags, PREPARE_FLOATBOXES) && getRendMethod() == erm_inline)
                    fbox->setRendMethod(erm_inline);
                else
                    fbox->setRendMethod(erm_block);

                // We want this floatBox to have no real style (and it surely
                // should not have the margins of the child), but it should probably
                // have the inherited properties of the node parent, just like the child
                // had them. We can't just copy the parent style into this floatBox, as
                // we don't want its non-inherited properties like background-color which
                // could be drawn over some other content if this float has some negative
                // margins.
                // So, we can't really do this:
                //    // Move float and display from me into my new fbox parent
                //    css_style_ref_t mystyle = getStyle();
                //    css_style_ref_t parentstyle = parent->getStyle();
                //    css_style_ref_t fboxstyle( new css_style_rec_t );
                //    copystyle(parentstyle, fboxstyle);
                //    fboxstyle->float_ = mystyle->float_;
                //    fboxstyle->display = mystyle->display;
                //    fbox->setStyle(fboxstyle);
                //    fbox->initNodeFont();
                //
                // Best to use lvrend.cpp setNodeStyle(), which will properly set
                // this new node style with inherited properties from its parent,
                // and we made it do this specific propagation of float_ and
                // display from its single children, only when it has styles
                // defined (so, only on initial loading and not on re-renderings).
                setNodeStyle(fbox, parent->getStyle(), parent->getFont());

                // We would have liked to reset style->float_ to none in the
                // node we moved in the floatBox, for correctness sake.
                //    css_style_ref_t mynewstyle( new css_style_rec_t );
                //    copystyle(mystyle, mynewstyle);
                //    mynewstyle->float_ = css_f_none;
                //    mynewstyle->display = css_d_block;
                //    setStyle(mynewstyle);
                //    initNodeFont();
                // Unfortunatly, we can't yet re-set a style while the DOM
                // is still being built (as we may be called during the loading
                // phase) without many font glitches.
                // So, we'll have a floatBox with float: that contains a span
                // or div with float: - the rendering code may have to check
                // for that: ->isFloatingBox() was added for that.
            }
        }
    }

    // (If a node is both inline-block and float: left/right, float wins.)
    if (BLOCK_RENDERING(rend_flags, BOX_INLINE_BLOCKS) && !handled_as_float) {
        // (Similar to what we do above for floats, but simpler.)
        // While loading the document, we want to put any element with
        // display: inline-block or inline-table inside an internal inlineBox
        // element with no margin in its style: this inlineBox's RenderRectAccessor
        // will have the width/height of the outer element (with margins inside),
        // while the RenderRectAccessor of the wrapped original element itself
        // will have the w/h of the element, including borders but excluding
        // margins (as it is done for all elements by crengine).
        // That makes out the following rules:
        // - a inlineBox has a single child: the original inline-block element.
        // - an element with style->display: inline-block/inline-table must be
        //   wrapped in a inlineBox, which will get the same style->vertical_align
        //   (happens in the initial document loading)
        // - if it already has a inlineBox parent, no need to do it again, just ensure
        //   the style->vertical_align are the same (happens when re-rendering)
        // - if the element has lost its style->display: inline-block (style tweak
        //   applied), or BOX_INLINE_BLOCKS disabled, as we can't remove the
        //   inlineBox (we can't modify the DOM once a cache has been made):
        //   the inlineBox and its children will both be set to erm_inline
        //   (but as ->display has changed, a full re-loading will be suggested
        //   to the user, and should probably be accepted).
        // - a inlineBox has ALWAYS ->display=css_d_inline and erm_method=erm_inline
        // - a inlineBox child keeps its original ->display, and may have
        //   erm_method = erm_final or erm_block (depending on its content)
        bool isInlineBlock = (d == css_d_inline_block || d == css_d_inline_table);
        bool isInlineBox = (getNodeId() == el_inlineBox);
        if (isInlineBlock || isInlineBox) {
            ldomNode* parent = getParentNode();
            bool isInlineBoxChild = (parent && (parent->getNodeId() == el_inlineBox));
            if (isInlineBox) {
                // Wrapping inlineBox already made
                if (getChildCount() != 1) {
                    CRLog::error("inlineBox with zero or more than one child");
                    crFatalError();
                }
                // Update inlineBox style according to child's one
                ldomNode* child = getChildNode(0);
                css_style_ref_t child_style = child->getStyle();
                css_style_ref_t my_style = getStyle();
                css_style_ref_t my_new_style(new css_style_rec_t);
                copystyle(my_style, my_new_style);
                if (child_style->display == css_d_inline_block || child_style->display == css_d_inline_table) {
                    my_new_style->display = css_d_inline; // become an inline wrapper
                    // We need it to have the vertical_align from the child
                    // (it's the only style we need for proper inline layout).
                    my_new_style->vertical_align = child_style->vertical_align;
                    setRendMethod(erm_inline);
                } else if (isEmbeddedBlockBoxingInlineBox(true)) {
                    my_new_style->display = css_d_inline; // wrap bogus "block among inlines" in inline
                    setRendMethod(erm_inline);
                } else if (child_style->display <= css_d_inline) {
                    my_new_style->display = css_d_inline; // wrap inline in inline
                    setRendMethod(erm_inline);
                } else if (child_style->display == css_d_none) {
                    my_new_style->display = css_d_none; // stay invisible
                    setRendMethod(erm_invisible);
                } else { // everything else must be wrapped by a block
                    my_new_style->display = css_d_block;
                    setRendMethod(erm_block);
                }
                setStyle(my_new_style);
                // When re-rendering, setNodeStyle() has already been called to set
                // our style and font, so no need for calling initNodeFont() here,
                // as we didn't change anything related to font in the style (and
                // calling it can cause a style hash mismatch for some reason).
            } else if (isInlineBoxChild) {
                // Already inlineBox'ed, nothing special to do
            } else if (parent) { // !isInlineBox && !isInlineBoxChild
                // Element with display: inline-block/inline-table, that has not yet
                // been wrapped in a inlineBox.
                // Replace this element with a inlineBox in its parent children collection,
                // and move it inside, as the single child of this inlineBox.
                int pos = getNodeIndex();
                ldomNode* ibox = parent->insertChildElement(pos, LXML_NS_NONE, el_inlineBox);
                parent->moveItemsTo(ibox, pos + 1, pos + 1); // move this element from parent into ibox
                ibox->setRendMethod(erm_inline);

                // We want this inlineBox to have no real style (and it surely
                // should not have the margins of the child), but it should probably
                // have the inherited properties of the node parent, just like the child
                // had them. We can't just copy the parent style into this inlineBox, as
                // we don't want its non-inherited properties like background-color which
                // could be drawn over some other content if this float has some negative
                // margins.
                // Best to use lvrend.cpp setNodeStyle(), which will properly set
                // this new node style with inherited properties from its parent,
                // and we made it do this specific propagation of vertical_align
                // from its single child, only when it has styles defined (so,
                // only on initial loading and not on re-renderings).
                setNodeStyle(ibox, parent->getStyle(), parent->getFont());
            }
        }
    }

#if MATHML_SUPPORT == 1
    if (getNodeId() == el_math && getDocument()->isBeingParsed()) {
        // Skip that uneeded work if the <math> element is "display:none"
        if (getRendMethod() != erm_invisible) {
            fixupMathMLMathElement(this);
        }
    }
#endif

    persist();
}

//#define DISABLE_STYLESHEET_REL
/// if stylesheet file name is set, and file is found, set stylesheet to its value
bool ldomNode::applyNodeStylesheet() {
#ifndef DISABLE_STYLESHEET_REL
    CRLog::trace("ldomNode::applyNodeStylesheet()");
    if (!getDocument()->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES)) // internal styles are disabled
        return false;

    if (getNodeId() != el_DocFragment && getNodeId() != el_body)
        return false;
    if (getNodeId() == el_DocFragment && getDocument()->getContainer().isNull())
        return false;

    // Here, we apply internal stylesheets that have been saved as attribute or
    // child element by the HTML parser for EPUB or plain HTML documents.

    // For epub documents, for each included .html in the epub, the first css
    // file link may have been put as the value of an added attribute to
    // the <DocFragment> element:
    //     <DocFragment StyleSheet="path to css file">
    //
    // For epub and html documents, the content of one or more <head><style>
    // elements, as well as all (only the 2nd++ for epub) linked css files,
    // with @import url(), have been put into an added child element:
    //     <DocFragment><stylesheet>css content</stylesheet><body>...</body></DocFragment>
    //     <body><stylesheet>css content</stylesheet>...</body>

    bool stylesheetChanged = false;

    if (getNodeId() == el_DocFragment && hasAttribute(attr_StyleSheet)) {
        getDocument()->_stylesheet.push();
        stylesheetChanged = getDocument()->parseStyleSheet(getAttributeValue(attr_StyleSheet));
        if (!stylesheetChanged)
            getDocument()->_stylesheet.pop();
    }
    if (getChildCount() > 0) {
        ldomNode* styleNode = getChildNode(0);

        if (styleNode && styleNode->getNodeId() == el_stylesheet) {
            if (false == stylesheetChanged) {
                getDocument()->_stylesheet.push();
            }
            if (getDocument()->parseStyleSheet(styleNode->getAttributeValue(attr_href),
                                               styleNode->getText())) {
                stylesheetChanged = true;
            } else if (false == stylesheetChanged) {
                getDocument()->_stylesheet.pop();
            }
        }
    }
    return stylesheetChanged;
#endif
    return false;
}

/// returns XPath segment for this element relative to parent element (e.g. "p[10]")
lString32 ldomNode::getXPathSegment() {
    if (isNull() || isRoot())
        return lString32::empty_str;
    ldomNode* parent = getParentNode();
    int cnt = parent->getChildCount();
    int index = 0;
    if (isElement()) {
        int id = getNodeId();
        for (int i = 0; i < cnt; i++) {
            ldomNode* node = parent->getChildNode(i);
            if (node == this) {
                return getNodeName() + "[" + fmt::decimal(index + 1) + "]";
            }
            if (node->isElement() && node->getNodeId() == id)
                index++;
        }
    } else {
        for (int i = 0; i < cnt; i++) {
            ldomNode* node = parent->getChildNode(i);
            if (node == this) {
                return "text()[" + lString32::itoa(index + 1) + "]";
            }
            if (node->isText())
                index++;
        }
    }
    return lString32::empty_str;
}

/// returns node level, 0 is root node
lUInt8 ldomNode::getNodeLevel() const {
    const ldomNode* node = this;
    int level = 0;
    for (; node; node = node->getParentNode())
        level++;
    return (lUInt8)level;
}

void ldomNode::onCollectionDestroy() {
    if (isNull())
        return;
    //CRLog::trace("ldomNode::onCollectionDestroy(%d) type=%d", this->_handle._dataIndex, TNTYPE);
    switch (TNTYPE) {
        case NT_TEXT:
            delete _data._text_ptr;
            _data._text_ptr = NULL;
            break;
        case NT_ELEMENT:
            // ???
            getDocument()->clearNodeStyle(_handle._dataIndex);
            delete NPELEM;
            NPELEM = NULL;
            break;
        case NT_PTEXT: // immutable (persistent) text node
            // do nothing
            break;
        case NT_PELEMENT: // immutable (persistent) element node
            // do nothing
            break;
    }
}

void ldomNode::destroy() {
    if (isNull())
        return;
    //CRLog::trace("ldomNode::destroy(%d) type=%d", this->_handle._dataIndex, TNTYPE);
    switch (TNTYPE) {
        case NT_TEXT:
            delete _data._text_ptr;
            break;
        case NT_ELEMENT: {
            getDocument()->clearNodeStyle(_handle._dataIndex);
            tinyElement* me = NPELEM;
            // delete children
            for (int i = 0; i < me->_children.length(); i++) {
                ldomNode* child = getDocument()->getTinyNode(me->_children[i]);
                if (child)
                    child->destroy();
            }
            delete me;
            NPELEM = NULL;
        }
            delete NPELEM;
            break;
        case NT_PTEXT:
            // disable removing from storage: to minimize modifications
            //_document->_textStorage->freeNode( _data._ptext_addr._addr );
            break;
        case NT_PELEMENT: // immutable (persistent) element node
        {
            ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            for (int i = 0; i < me->childCount; i++)
                getDocument()->getTinyNode(me->children[i])->destroy();
            getDocument()->clearNodeStyle(_handle._dataIndex);
            //            getDocument()->_styles.release( _data._pelem._styleIndex );
            //            getDocument()->_fonts.release( _data._pelem._fontIndex );
            //            _data._pelem._styleIndex = 0;
            //            _data._pelem._fontIndex = 0;
            getDocument()->_elemStorage->freeNode(_data._pelem_addr);
        } break;
    }
    getDocument()->recycleTinyNode(_handle._dataIndex);
}

/// returns index of child node by dataIndex
int ldomNode::getChildIndex(lUInt32 dataIndex) const {
    // was here and twice below: dataIndex &= 0xFFFFFFF0;
    // The lowest bits of a dataIndex carry properties about the node:
    //   bit 0: 0 = text node / 1 = element node
    //   bit 1: 0 = mutable node / 1 = immutable (persistent, cached)
    // (So, all Text nodes have an even dataIndex, and Element nodes
    // all have a odd dataIndex.)
    // This '& 0xFFFFFFF0' was to clear these properties so a same
    // node can be found if these properties change (mostly useful
    // with mutable<>persistent).
    // But text nodes and Element nodes use different independant counters
    // (see tinyNodeCollection::allocTinyNode(): _elemCount++, _textCount++)
    // and we may have a text node with dataIndex 8528, and an element
    // node with dataIndex 8529, that would be confused with each other
    // if we use 0xFFFFFFF0.
    // This could cause finding the wrong node, and strange side effects.
    // With '& 0xFFFFFFF1' keep the lowest bit.
    dataIndex &= 0xFFFFFFF1;
    ASSERT_NODE_NOT_NULL;
    int parentIndex = -1;
    switch (TNTYPE) {
        case NT_ELEMENT: {
            tinyElement* me = NPELEM;
            for (int i = 0; i < me->_children.length(); i++) {
                if ((me->_children[i] & 0xFFFFFFF1) == dataIndex) {
                    // found
                    parentIndex = i;
                    break;
                }
            }
        } break;
        case NT_PELEMENT: {
            ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            for (int i = 0; i < me->childCount; i++) {
                if ((me->children[i] & 0xFFFFFFF1) == dataIndex) {
                    // found
                    parentIndex = i;
                    break;
                }
            }
        } break;
        case NT_PTEXT: // immutable (persistent) text node
        case NT_TEXT:
            break;
    }
    return parentIndex;
}

/// returns index of node inside parent's child collection
int ldomNode::getNodeIndex() const {
    ASSERT_NODE_NOT_NULL;
    ldomNode* parent = getParentNode();
    if (parent)
        return parent->getChildIndex(getDataIndex());
    return 0;
}

/// returns true if node is document's root
bool ldomNode::isRoot() const {
    ASSERT_NODE_NOT_NULL;
    switch (TNTYPE) {
        case NT_ELEMENT:
            return !NPELEM->_parentNode;
        case NT_PELEMENT: // immutable (persistent) element node
        {
            ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            return me->parentIndex == 0;
        } break;
        case NT_PTEXT: // immutable (persistent) text node
        {
            return getDocument()->_textStorage->getParent(_data._ptext_addr) == 0;
        }
        case NT_TEXT:
            return _data._text_ptr->getParentIndex() == 0;
    }
    return false;
}

/// call to invalidate cache if persistent node content is modified
void ldomNode::modified() {
    if (isPersistent()) {
        if (isElement())
            getDocument()->_elemStorage->modified(_data._pelem_addr);
        else
            getDocument()->_textStorage->modified(_data._ptext_addr);
    }
}

/// changes parent of item
void ldomNode::setParentNode(ldomNode* parent) {
    ASSERT_NODE_NOT_NULL;
#ifdef TRACE_AUTOBOX
    if (getParentNode() != NULL && parent != NULL)
        CRLog::trace("Changing parent of %d from %d to %d", getDataIndex(), getParentNode()->getDataIndex(), parent->getDataIndex());
#endif
    switch (TNTYPE) {
        case NT_ELEMENT:
            NPELEM->_parentNode = parent;
            break;
        case NT_PELEMENT: // immutable (persistent) element node
        {
            lUInt32 parentIndex = parent->_handle._dataIndex;
            ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            if (me->parentIndex != (int)parentIndex) {
                me->parentIndex = parentIndex;
                modified();
            }
        } break;
        case NT_PTEXT: // immutable (persistent) text node
        {
            lUInt32 parentIndex = parent->_handle._dataIndex;
            getDocument()->_textStorage->setParent(_data._ptext_addr, parentIndex);
            //_data._ptext_addr._parentIndex = parentIndex;
            //_document->_textStorage->setTextParent( _data._ptext_addr._addr, parentIndex );
        } break;
        case NT_TEXT: {
            lUInt32 parentIndex = parent->_handle._dataIndex;
            _data._text_ptr->setParentIndex(parentIndex);
        } break;
    }
}

/// returns dataIndex of node's parent, 0 if no parent
int ldomNode::getParentIndex() const {
    ASSERT_NODE_NOT_NULL;

    switch (TNTYPE) {
        case NT_ELEMENT:
            return NPELEM->_parentNode ? NPELEM->_parentNode->getDataIndex() : 0;
        case NT_PELEMENT: // immutable (persistent) element node
        {
            ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            return me->parentIndex;
        } break;
        case NT_PTEXT: // immutable (persistent) text node
            return getDocument()->_textStorage->getParent(_data._ptext_addr);
        case NT_TEXT:
            return _data._text_ptr->getParentIndex();
    }
    return 0;
}

/// returns pointer to parent node, NULL if node has no parent
ldomNode* ldomNode::getParentNode() const {
    ASSERT_NODE_NOT_NULL;
    int parentIndex = 0;
    switch (TNTYPE) {
        case NT_ELEMENT:
            return NPELEM->_parentNode;
        case NT_PELEMENT: // immutable (persistent) element node
        {
            ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            parentIndex = me->parentIndex;
        } break;
        case NT_PTEXT: // immutable (persistent) text node
            parentIndex = getDocument()->_textStorage->getParent(_data._ptext_addr);
            break;
        case NT_TEXT:
            parentIndex = _data._text_ptr->getParentIndex();
            break;
    }
    return parentIndex ? getTinyNode(parentIndex) : NULL;
}

/// returns true child node is element
bool ldomNode::isChildNodeElement(lUInt32 index) const {
    ASSERT_NODE_NOT_NULL;
    if (!isPersistent()) {
        // element
        tinyElement* me = NPELEM;
        int n = me->_children[index];
        return ((n & 1) == 1);
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        int n = me->children[index];
        return ((n & 1) == 1);
    }
}

/// returns true child node is text
bool ldomNode::isChildNodeText(lUInt32 index) const {
    ASSERT_NODE_NOT_NULL;
    if (!isPersistent()) {
        // element
        tinyElement* me = NPELEM;
        int n = me->_children[index];
        return ((n & 1) == 0);
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        int n = me->children[index];
        return ((n & 1) == 0);
    }
}

/// returns child node by index, NULL if node with this index is not element or nodeTag!=0 and element node name!=nodeTag
ldomNode* ldomNode::getChildElementNode(lUInt32 index, const lChar32* nodeTag) const {
    lUInt16 nodeId = getDocument()->getElementNameIndex(nodeTag);
    return getChildElementNode(index, nodeId);
}

/// returns child node by index, NULL if node with this index is not element or nodeId!=0 and element node id!=nodeId
ldomNode* ldomNode::getChildElementNode(lUInt32 index, lUInt16 nodeId) const {
    ASSERT_NODE_NOT_NULL;
    ldomNode* res = NULL;
    if (!isPersistent()) {
        // element
        tinyElement* me = NPELEM;
        int n = me->_children[index];
        if ((n & 1) == 0) // not element
            return NULL;
        res = getTinyNode(n);
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        int n = me->children[index];
        if ((n & 1) == 0) // not element
            return NULL;
        res = getTinyNode(n);
    }
    if (res && nodeId != 0 && res->getNodeId() != nodeId)
        res = NULL;
    return res;
}

/// returns child node by index
ldomNode* ldomNode::getChildNode(lUInt32 index) const {
    ASSERT_NODE_NOT_NULL;
    if (!isPersistent()) {
        // element
        tinyElement* me = NPELEM;
        return getTinyNode(me->_children[index]);
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        return getTinyNode(me->children[index]);
    }
}

/// returns element child count
int ldomNode::getChildCount() const {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return 0;
    if (!isPersistent()) {
        // element
        tinyElement* me = NPELEM;
        return me->_children.length();
    } else {
        // persistent element
        // persistent element
        {
            ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            //            if ( me==NULL ) { // DEBUG
            //                me = _document->_elemStorage->getElem( _data._pelem_addr );
            //            }
            return me->childCount;
        }
    }
}

/// returns element attribute count
int ldomNode::getAttrCount() const {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return 0;
    if (!isPersistent()) {
        // element
        tinyElement* me = NPELEM;
        return me->_attrs.length();
    } else {
        // persistent element
        {
            ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            return me->attrCount;
        }
    }
}

/// returns attribute value by attribute name id and namespace id
const lString32& ldomNode::getAttributeValue(lUInt16 nsid, lUInt16 id) const {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return lString32::empty_str;
    if (!isPersistent()) {
        // element
        tinyElement* me = NPELEM;
        lUInt32 valueId = me->_attrs.get(nsid, id);
        if (valueId == LXML_ATTR_VALUE_NONE)
            return lString32::empty_str;
        return getDocument()->getAttrValue(valueId);
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        lUInt32 valueId = me->getAttrValueId(nsid, id);
        if (valueId == LXML_ATTR_VALUE_NONE)
            return lString32::empty_str;
        return getDocument()->getAttrValue(valueId);
    }
}

/// returns attribute value by attribute name and namespace
const lString32& ldomNode::getAttributeValue(const lChar32* nsName, const lChar32* attrName) const {
    ASSERT_NODE_NOT_NULL;
    lUInt16 nsId = (nsName && nsName[0]) ? getDocument()->getNsNameIndex(nsName) : LXML_NS_ANY;
    lUInt16 attrId = getDocument()->getAttrNameIndex(attrName);
    return getAttributeValue(nsId, attrId);
}

/// returns attribute value by attribute name and namespace
const lString32& ldomNode::getAttributeValue(const lChar8* nsName, const lChar8* attrName) const {
    ASSERT_NODE_NOT_NULL;
    lUInt16 nsId = (nsName && nsName[0]) ? getDocument()->getNsNameIndex(nsName) : LXML_NS_ANY;
    lUInt16 attrId = getDocument()->getAttrNameIndex(attrName);
    return getAttributeValue(nsId, attrId);
}

/// returns attribute by index
const lxmlAttribute* ldomNode::getAttribute(lUInt32 index) const {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return NULL;
    if (!isPersistent()) {
        // element
        tinyElement* me = NPELEM;
        return me->_attrs[index];
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        return me->attr(index);
    }
}

/// returns true if element node has attribute with specified name id and namespace id
bool ldomNode::hasAttribute(lUInt16 nsid, lUInt16 id) const {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return false;
    if (!isPersistent()) {
        // element
        tinyElement* me = NPELEM;
        lUInt32 valueId = me->_attrs.get(nsid, id);
        return (valueId != LXML_ATTR_VALUE_NONE);
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        return (me->findAttr(nsid, id) != NULL);
    }
}

/// returns attribute name by index
const lString32& ldomNode::getAttributeName(lUInt32 index) const {
    ASSERT_NODE_NOT_NULL;
    const lxmlAttribute* attr = getAttribute(index);
    if (attr)
        return getDocument()->getAttrName(attr->id);
    return lString32::empty_str;
}

/// sets attribute value
void ldomNode::setAttributeValue(lUInt16 nsid, lUInt16 id, const lChar32* value) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return;
    lUInt32 valueIndex = getDocument()->getAttrValueIndex(value);
    if (isPersistent()) {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        lxmlAttribute* attr = me->findAttr(nsid, id);
        if (attr) {
            attr->index = valueIndex;
            modified();
            return;
        }
        // else: convert to modifable and continue as non-persistent
        modify();
    }
    // element
    tinyElement* me = NPELEM;
    me->_attrs.set(nsid, id, valueIndex);
    if (nsid == LXML_NS_NONE)
        getDocument()->onAttributeSet(id, valueIndex, this);
}

/// returns attribute value by attribute name id, looking at children if needed
const lString32& ldomNode::getFirstInnerAttributeValue(lUInt16 nsid, lUInt16 id) const {
    ASSERT_NODE_NOT_NULL;
    if (hasAttribute(nsid, id))
        return getAttributeValue(nsid, id);
    ldomNode* n = (ldomNode*)this;
    if (n->isElement() && n->getChildCount() > 0) {
        int nextChildIndex = 0;
        n = n->getChildNode(nextChildIndex);
        while (true) {
            // Check only the first time we met a node (nextChildIndex == 0)
            // and not when we get back to it from a child to process next sibling
            if (nextChildIndex == 0) {
                if (n->isElement() && n->hasAttribute(nsid, id))
                    return n->getAttributeValue(nsid, id);
            }
            // Process next child
            if (n->isElement() && nextChildIndex < n->getChildCount()) {
                n = n->getChildNode(nextChildIndex);
                nextChildIndex = 0;
                continue;
            }
            // No more child, get back to parent and have it process our sibling
            nextChildIndex = n->getNodeIndex() + 1;
            n = n->getParentNode();
            if (!n) // back to root node
                break;
            if (n == this && nextChildIndex >= n->getChildCount())
                // back to this node, and done with its children
                break;
        }
    }
    return lString32::empty_str;
}

/// returns element type structure pointer if it was set in document for this element name
const css_elem_def_props_t* ldomNode::getElementTypePtr() {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return NULL;
    if (!isPersistent()) {
        // element
        const css_elem_def_props_t* res = getDocument()->getElementTypePtr(NPELEM->_id);
        //        if ( res && res->is_object ) {
        //            CRLog::trace("Object found");
        //        }
        return res;
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        const css_elem_def_props_t* res = getDocument()->getElementTypePtr(me->id);
        //        if ( res && res->is_object ) {
        //            CRLog::trace("Object found");
        //        }
        return res;
    }
}

/// returns element name id
lUInt16 ldomNode::getNodeId() const {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return 0;
    if (!isPersistent()) {
        // element
        return NPELEM->_id;
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        return me->id;
    }
}

/// returns element namespace id
lUInt16 ldomNode::getNodeNsId() const {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return 0;
    if (!isPersistent()) {
        // element
        return NPELEM->_nsid;
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        return me->nsid;
    }
}

/// replace element name id with another value
void ldomNode::setNodeId(lUInt16 id) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return;
    if (!isPersistent()) {
        // element
        NPELEM->_id = id;
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        me->id = id;
        modified();
    }
}

/// returns element name
const lString32& ldomNode::getNodeName() const {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return lString32::empty_str;
    if (!isPersistent()) {
        // element
        return getDocument()->getElementName(NPELEM->_id);
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        return getDocument()->getElementName(me->id);
    }
}

/// returns element name
bool ldomNode::isNodeName(const char* s) const {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return false;
    lUInt16 index = getDocument()->findElementNameIndex(s);
    if (!index)
        return false;
    if (!isPersistent()) {
        // element
        return index == NPELEM->_id;
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        return index == me->id;
    }
}

/// returns element namespace name
const lString32& ldomNode::getNodeNsName() const {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return lString32::empty_str;
    if (!isPersistent()) {
        // element
        return getDocument()->getNsName(NPELEM->_nsid);
    } else {
        // persistent element
        ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
        return getDocument()->getNsName(me->nsid);
    }
}

/// returns text node text as wide string
lString32 ldomNode::getText(lChar32 blockDelimiter, int maxSize) const {
    ASSERT_NODE_NOT_NULL;
    switch (TNTYPE) {
        case NT_PELEMENT:
        case NT_ELEMENT: {
            lString32 txt;
            unsigned cc = getChildCount();
            for (unsigned i = 0; i < cc; i++) {
                ldomNode* child = getChildNode(i);
                txt += child->getText(blockDelimiter, maxSize);
                if (maxSize != 0 && txt.length() > maxSize)
                    break;
                if (i >= cc - 1)
                    break;
                if (blockDelimiter && child->isElement()) {
                    if (!child->getStyle().isNull() && child->getStyle()->display == css_d_block)
                        txt << blockDelimiter;
                }
            }
            return txt;
        } break;
        case NT_PTEXT:
            return Utf8ToUnicode(getDocument()->_textStorage->getText(_data._ptext_addr));
        case NT_TEXT:
            return _data._text_ptr->getText32();
    }
    return lString32::empty_str;
}

/// returns text node text as utf8 string
lString8 ldomNode::getText8(lChar8 blockDelimiter, int maxSize) const {
    ASSERT_NODE_NOT_NULL;
    switch (TNTYPE) {
        case NT_ELEMENT:
        case NT_PELEMENT: {
            lString8 txt;
            int cc = getChildCount();
            for (int i = 0; i < cc; i++) {
                ldomNode* child = getChildNode(i);
                txt += child->getText8(blockDelimiter, maxSize);
                if (maxSize != 0 && txt.length() > maxSize)
                    break;
                if (i >= getChildCount() - 1)
                    break;
                if (blockDelimiter && child->isElement()) {
                    if (child->getStyle()->display == css_d_block)
                        txt << blockDelimiter;
                }
            }
            return txt;
        } break;
        case NT_PTEXT:
            return getDocument()->_textStorage->getText(_data._ptext_addr);
        case NT_TEXT:
            return _data._text_ptr->getText();
    }
    return lString8::empty_str;
}

/// sets text node text as wide string
void ldomNode::setText(lString32 str) {
    ASSERT_NODE_NOT_NULL;
    switch (TNTYPE) {
        case NT_ELEMENT:
            readOnlyError();
            break;
        case NT_PELEMENT:
            readOnlyError();
            break;
        case NT_PTEXT: {
            // convert persistent text to mutable
            lUInt32 parentIndex = getDocument()->_textStorage->getParent(_data._ptext_addr);
            getDocument()->_textStorage->freeNode(_data._ptext_addr);
            _data._text_ptr = new ldomTextNode(parentIndex, UnicodeToUtf8(str));
            // change type from PTEXT to TEXT
            _handle._dataIndex = (_handle._dataIndex & ~0xF) | NT_TEXT;
        } break;
        case NT_TEXT: {
            _data._text_ptr->setText(str);
        } break;
    }
}

/// sets text node text as utf8 string
void ldomNode::setText8(lString8 utf8) {
    ASSERT_NODE_NOT_NULL;
    switch (TNTYPE) {
        case NT_ELEMENT:
            readOnlyError();
            break;
        case NT_PELEMENT:
            readOnlyError();
            break;
        case NT_PTEXT: {
            // convert persistent text to mutable
            lUInt32 parentIndex = getDocument()->_textStorage->getParent(_data._ptext_addr);
            getDocument()->_textStorage->freeNode(_data._ptext_addr);
            _data._text_ptr = new ldomTextNode(parentIndex, utf8);
            // change type from PTEXT to TEXT
            _handle._dataIndex = (_handle._dataIndex & ~0xF) | NT_TEXT;
        } break;
        case NT_TEXT: {
            _data._text_ptr->setText(utf8);
        } break;
    }
}

/// returns node absolute rectangle
void ldomNode::getAbsRect(lvRect& rect, bool inner) {
    ASSERT_NODE_NOT_NULL;
    ldomNode* node = this;
    RenderRectAccessor fmt(node);
    rect.left = fmt.getX();
    rect.top = fmt.getY();
    rect.right = fmt.getWidth();
    rect.bottom = fmt.getHeight();
    if (inner && RENDER_RECT_HAS_FLAG(fmt, INNER_FIELDS_SET)) {
        // This flag is set only when in enhanced rendering mode, and
        // only on erm_final nodes.
        rect.left += fmt.getInnerX();     // add padding left
        rect.top += fmt.getInnerY();      // add padding top
        rect.right = fmt.getInnerWidth(); // replace by inner width
    }
    node = node->getParentNode();
    for (; node; node = node->getParentNode()) {
        RenderRectAccessor fmt(node);
        rect.left += fmt.getX();
        rect.top += fmt.getY();
        if (RENDER_RECT_HAS_FLAG(fmt, INNER_FIELDS_SET)) {
            // getAbsRect() is mostly used on erm_final nodes. So,
            // if we meet another erm_final node in our parent, we are
            // probably an embedded floatBox or inlineBox. Embedded
            // floatBoxes or inlineBoxes are positioned according
            // to the inner LFormattedText, so we need to account
            // for these padding shifts.
            rect.left += fmt.getInnerX(); // add padding left
            rect.top += fmt.getInnerY();  // add padding top
        }
    }
    rect.bottom += rect.top;
    rect.right += rect.left;
}

/// returns render data structure
void ldomNode::getRenderData(lvdomElementFormatRec& dst) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement()) {
        dst.clear();
        return;
    }
    getDocument()->_rectStorage->getRendRectData(_handle._dataIndex, &dst);
}

/// sets new value for render data structure
void ldomNode::setRenderData(lvdomElementFormatRec& newData) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return;
    getDocument()->_rectStorage->setRendRectData(_handle._dataIndex, &newData);
}

/// sets node rendering structure pointer
void ldomNode::clearRenderData() {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return;
    lvdomElementFormatRec rec;
    getDocument()->_rectStorage->setRendRectData(_handle._dataIndex, &rec);
}
/// reset node rendering structure pointer for sub-tree
void ldomNode::clearRenderDataRecursive() {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return;
    lvdomElementFormatRec rec;
    getDocument()->_rectStorage->setRendRectData(_handle._dataIndex, &rec);
    int cnt = getChildCount();
    for (int i = 0; i < cnt; i++) {
        ldomNode* child = getChildNode(i);
        if (child->isElement()) {
            child->clearRenderDataRecursive();
        }
    }
}

/// calls specified function recursively for all elements of DOM tree, children before parent
void ldomNode::recurseElementsDeepFirst(void (*pFun)(ldomNode* node)) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return;
    int cnt = getChildCount();
    for (int i = 0; i < cnt; i++) {
        ldomNode* child = getChildNode(i);
        if (child && child->isElement()) {
            child->recurseElementsDeepFirst(pFun);
        }
    }
    pFun(this);
}

static void updateRendMethod(ldomNode* node) {
    node->initNodeRendMethod();
    // Also clean up node previous positionings (they were set while in
    // a previous page drawing phase), that could otherwise have negative
    // impact on the coming rendering (noticeable with table elements).
    RenderRectAccessor fmt(node);
    fmt.clear();
    fmt.push();
}

/// init render method for the whole subtree
void ldomNode::initNodeRendMethodRecursive() {
    recurseElementsDeepFirst(updateRendMethod);
}

#if 0
static void updateStyleData( ldomNode * node )
{
    if ( node->getNodeId()==el_DocFragment )
        node->applyNodeStylesheet();
    node->initNodeStyle();
}
#endif

static void updateStyleDataRecursive(ldomNode* node, LVDocViewCallback* progressCallback, int& lastProgressPercent) {
    if (!node->isElement())
        return;
    bool styleSheetChanged = false;

    // DocFragment (for epub) and body (for html) may hold some stylesheet
    // as first child or a link to stylesheet file in attribute
    if (node->getNodeId() == el_DocFragment || node->getNodeId() == el_body) {
        styleSheetChanged = node->applyNodeStylesheet();
        // We don't have access to much metric to show the progress of
        // this recursive phase. Do that anyway as we progress among
        // the collection of DocFragments.
        if (progressCallback && node->getNodeId() == el_DocFragment) {
            int nbDocFragments = node->getParentNode()->getChildCount();
            if (nbDocFragments == 0) // should not happen (but avoid clang-tidy warning)
                nbDocFragments = 1;
            int percent = 100 * node->getNodeIndex() / nbDocFragments;
            if (percent != lastProgressPercent) {
                progressCallback->OnNodeStylesUpdateProgress(percent);
                lastProgressPercent = percent;
            }
        }
    }

    node->initNodeStyle();
    int n = node->getChildCount();
    for (int i = 0; i < n; i++) {
        ldomNode* child = node->getChildNode(i);
        if (child && child->isElement())
            updateStyleDataRecursive(child, progressCallback, lastProgressPercent);
    }
    if (styleSheetChanged)
        node->getDocument()->getStyleSheet()->pop();
}

/// init render method for the whole subtree
void ldomNode::initNodeStyleRecursive(LVDocViewCallback* progressCallback) {
    if (progressCallback)
        progressCallback->OnNodeStylesUpdateStart();
    getDocument()->_fontMap.clear();
    int lastProgressPercent = -1;
    updateStyleDataRecursive(this, progressCallback, lastProgressPercent);
    //recurseElements( updateStyleData );
    if (progressCallback)
        progressCallback->OnNodeStylesUpdateEnd();
}

/// calls specified function recursively for all elements of DOM tree
void ldomNode::recurseElements(void (*pFun)(ldomNode* node)) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return;
    pFun(this);
    int cnt = getChildCount();
    for (int i = 0; i < cnt; i++) {
        ldomNode* child = getChildNode(i);
        if (child->isElement()) {
            child->recurseElements(pFun);
        }
    }
}

/// calls specified function recursively for all elements of DOM tree
void ldomNode::recurseMatchingElements(void (*pFun)(ldomNode* node), bool (*matchFun)(ldomNode* node)) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return;
    if (!matchFun(this)) {
        return;
    }
    pFun(this);
    int cnt = getChildCount();
    for (int i = 0; i < cnt; i++) {
        ldomNode* child = getChildNode(i);
        if (child->isElement()) {
            child->recurseMatchingElements(pFun, matchFun);
        }
    }
}

/// calls specified function recursively for all nodes of DOM tree
void ldomNode::recurseNodes(void (*pFun)(ldomNode* node)) {
    ASSERT_NODE_NOT_NULL;
    pFun(this);
    if (isElement()) {
        int cnt = getChildCount();
        for (int i = 0; i < cnt; i++) {
            ldomNode* child = getChildNode(i);
            child->recurseNodes(pFun);
        }
    }
}

/// returns first text child element
ldomNode* ldomNode::getFirstTextChild(bool skipEmpty) {
    ASSERT_NODE_NOT_NULL;
    if (isText()) {
        if (!skipEmpty)
            return this;
        lString32 txt = getText();
        bool nonSpaceFound = false;
        for (int i = 0; i < txt.length(); i++) {
            lChar32 ch = txt[i];
            if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
                nonSpaceFound = true;
                break;
            }
        }
        if (nonSpaceFound)
            return this;
        return NULL;
    }
    for (int i = 0; i < (int)getChildCount(); i++) {
        ldomNode* p = getChildNode(i)->getFirstTextChild(skipEmpty);
        if (p)
            return p;
    }
    return NULL;
}

/// returns last text child element
ldomNode* ldomNode::getLastTextChild() {
    ASSERT_NODE_NOT_NULL;
    if (isText())
        return this;
    else {
        for (int i = (int)getChildCount() - 1; i >= 0; i--) {
            ldomNode* p = getChildNode(i)->getLastTextChild();
            if (p)
                return p;
        }
    }
    return NULL;
}

/// find node by coordinates of point in formatted document
ldomNode* ldomNode::elementFromPoint(lvPoint pt, int direction, bool strict_bounds_checking) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return NULL;
    ldomNode* enode = this;
    lvdom_element_render_method rm = enode->getRendMethod();
    if (rm == erm_invisible) {
        return NULL;
    }

    if (rm == erm_inline) {
        // We shouldn't meet erm_inline here, as our purpose is to return
        // a final node (so, the container of inlines), and not look further
        // (it's ldomDocument::createXPointer(pt) job to look at this final
        // node rendered content to find the exact text node and char at pt).
        // Except in the "pt.y is inside the box bottom overflow" case below,
        // and that box is erm_final (see there for more comments).
        // We should navigate all the erm_inline nodes, looking for
        // non-erm_inline ones that may be in that overflow and containt pt.
        // erm_inline nodes don't have a RenderRectAccessor(), so their x/y
        // shifts are 0, and any inner block node had its RenderRectAccessor
        // x/y offsets positioned related to the final block. So, no need
        // to shift pt: just recursively call elementFromPoint() as-is,
        // and we'll be recursively navigating inline nodes here.
        int count = getChildCount();
        for (int i = 0; i < count; i++) {
            ldomNode* p = getChildNode(i);
            ldomNode* e = p->elementFromPoint(pt, direction);
            if (e) // found it!
                return e;
        }
        return NULL; // nothing found
    }

    RenderRectAccessor fmt(this);

    if (BLOCK_RENDERING_N(this, ENHANCED)) {
        // In enhanced rendering mode, because of collapsing of vertical margins
        // and the fact that we did not update style margins to their computed
        // values, a children box with margins can overlap its parent box, if
        // the child bigger margin collapsed with the parent smaller margin.
        // So, if we ignore the margins, there can be holes along the vertical
        // axis (these holes are the collapsed margins). But the content boxes
        // (without margins) don't overlap.
        if (direction >= PT_DIR_EXACT) { // PT_DIR_EXACT or PT_DIR_SCAN_FORWARD*
            // We get the parent node's children in ascending order
            // It could just be:
            //   if ( pt.y >= fmt.getY() + fmt.getHeight() )
            //       // Box fully before pt.y: not a candidate, next one may be
            //       return NULL;
            // but, because of possible floats overflowing their container element,
            // and we want to check if pt is inside any of them, we directly
            // check with bottom overflow included (just to avoid 2 tests
            // in the most common case when there is no overflow).
            if (pt.y >= fmt.getY() + fmt.getHeight() + fmt.getBottomOverflow()) {
                // Box (with overflow) fully before pt.y: not a candidate, next one may be
                return NULL;
            }
            if (pt.y >= fmt.getY() + fmt.getHeight()) { // pt.y is inside the box bottom overflow
                // Get back absolute coordinates of pt
                lvRect rc;
                getParentNode()->getAbsRect(rc);
                lvPoint pt0 = lvPoint(rc.left + pt.x, rc.top + pt.y);
                // Check each of this element's children if pt is inside it (so, we'll
                // go by here for each of them that has some overflow too, and that
                // contributed to making this element's overflow.)
                // Note that if this node is erm_final, its bottom overflow must have
                // been set by some inner embedded float. But this final block's children
                // are erm_inline, and the float might be deep among inlines' children.
                // erm_inline nodes don't have their RenderRectAccessor set, so the
                // bottom overflow is not propagated thru them, and we would be in
                // the above case ("Box (with overflow) fully before pt.y"), not
                // looking at inlines' children. We handle this case above (at the
                // start of this function) by looking at erm_inline's children for
                // non-erm_inline nodes before checking any x/y or bottom overflow.
                int count = getChildCount();
                for (int i = 0; i < count; i++) {
                    ldomNode* p = getChildNode(i);
                    // Find an inner erm_final element that has pt in it: for now, it can
                    // only be a float. Use PT_DIR_EXACT to really check for x boundaries.
                    ldomNode* e = p->elementFromPoint(lvPoint(pt.x - fmt.getX(), pt.y - fmt.getY()), PT_DIR_EXACT);
                    if (e) {
                        // Just to be sure, as elementFromPoint() may be a bit fuzzy in its
                        // checks, double check that pt is really inside that e rect.
                        lvRect erc;
                        e->getAbsRect(erc);
                        if (erc.isPointInside(pt0)) {
                            return e; // return this inner erm_final
                        }
                    }
                }
                return NULL; // Nothing found in the overflow
            }
            // There is one special case to skip: floats that may have been
            // positioned after their normal y (because of clear:, or because
            // of not enough width). Their following non-float siblings (after
            // in the HTML/DOM tree) may have a lower fmt.getY().
            if (isFloatingBox() && pt.y < fmt.getY()) {
                // Float starts after pt.y: next non-float siblings may contain pt.y
                return NULL;
            }
            // When children of the parent node have been re-ordered, we can't
            // trust the ordering, and if pt.y is before fmt.getY(), we might
            // still find it in a next node that have been re-ordered before
            // this one for rendering.
            // Note: for now, happens only with re-ordered table rows, so
            // we're only ensuring it here for y. This check might have to
            // also be done elsewhere in this function when we use it for
            // other things.
            if (strict_bounds_checking && pt.y < fmt.getY()) {
                // Box fully after pt.y: not a candidate, next one
                // (if reordered) may be
                return NULL;
            }
            // pt.y is inside the box (without overflows), go on with it.
            // Note: we don't check for next elements which may have a top
            // overflow and have pt.y inside it, because it would be a bit
            // more twisted here, and it's less common that floats overflow
            // their container's top (they need to have negative margins).
        } else { // PT_DIR_SCAN_BACKWARD*
            // We get the parent node's children in descending order
            if (pt.y < fmt.getY()) {
                // Box fully after pt.y: not a candidate, next one may be
                return NULL;
            }
            if (strict_bounds_checking && pt.y >= fmt.getY() + fmt.getHeight()) {
                // Box fully before pt.y: not a candidate, next one
                // (if reordered) may be
                return NULL;
            }
        }
    } else {
        // In legacy rendering mode, all boxes (with their margins added) touch
        // each other, and the boxes of children are fully contained (with
        // their margins added) in their parent box.

        // Styles margins set on <TR>, <THEAD> and the like are ignored
        // by table layout algorithm (as per CSS specs)
        // (erm_table_row_group, erm_table_header_group, erm_table_footer_group, erm_table_row)
        bool ignore_margins = rm >= erm_table_row_group && rm <= erm_table_row;

        int top_margin = ignore_margins ? 0 : lengthToPx(enode, enode->getStyle()->margin[2], fmt.getWidth());
        if (pt.y < fmt.getY() - top_margin) {
            if (direction >= PT_DIR_SCAN_FORWARD && rm == erm_final)
                return this;
            return NULL;
        }
        int bottom_margin = ignore_margins ? 0 : lengthToPx(enode, enode->getStyle()->margin[3], fmt.getWidth());
        if (pt.y >= fmt.getY() + fmt.getHeight() + bottom_margin) {
            if (direction <= PT_DIR_SCAN_BACKWARD && rm == erm_final)
                return this;
            return NULL;
        }
    }

    if (direction == PT_DIR_EXACT) {
        // (We shouldn't check for pt.x when we are given PT_DIR_SCAN_*.
        // In full text search, we might not find any and get locked
        // on some page.)
        if (pt.x >= fmt.getX() + fmt.getWidth()) {
            return NULL;
        }
        if (pt.x < fmt.getX()) {
            return NULL;
        }
        // We now do this above check in all cases.
        // Previously:
        //
        //   We also don't need to do it if pt.x=0, which is often used
        //   to get current page top or range xpointers.
        //   We are given a x>0 when tap/hold to highlight text or find
        //   a link, and checking x vs fmt.x and width allows for doing
        //   that correctly in 2nd+ table cells.
        //
        //   No need to check if ( pt.x < fmt.getX() ): we probably
        //   meet the multiple elements that can be formatted on a same
        //   line in the order they appear as children of their parent,
        //   we can simply just ignore those who end before our pt.x.
        //   But check x if we happen to be on a floating node (which,
        //   with float:right, can appear first in the DOM but be
        //   displayed at a higher x)
        //    if ( pt.x < fmt.getX() && enode->isFloatingBox() ) {
        //        return NULL;
        //    }
        // This is no more true, now that we support RTL tables and
        // we can meet cells in the reverse of their logical order.
        // We could add more conditions (like parentNode->getRendMethod()>=erm_table),
        // but let's just check this in all cases when direction=0.
    }
    if (rm == erm_final) {
        // Final node, that's what we looked for
        return this;
    }
    // Not a final node, but a block container node that must contain
    // the final node we look for: check its children.
    int count = getChildCount();
    strict_bounds_checking = RENDER_RECT_HAS_FLAG(fmt, CHILDREN_RENDERING_REORDERED);
    if (direction >= PT_DIR_EXACT) { // PT_DIR_EXACT or PT_DIR_SCAN_FORWARD*
        for (int i = 0; i < count; i++) {
            ldomNode* p = getChildNode(i);
            ldomNode* e = p->elementFromPoint(lvPoint(pt.x - fmt.getX(), pt.y - fmt.getY()), direction, strict_bounds_checking);
            if (e)
                return e;
        }
    } else {
        for (int i = count - 1; i >= 0; i--) {
            ldomNode* p = getChildNode(i);
            ldomNode* e = p->elementFromPoint(lvPoint(pt.x - fmt.getX(), pt.y - fmt.getY()), direction, strict_bounds_checking);
            if (e)
                return e;
        }
    }
    return this;
}

/// find final node by coordinates of point in formatted document
ldomNode* ldomNode::finalBlockFromPoint(lvPoint pt) {
    ASSERT_NODE_NOT_NULL;
    ldomNode* elem = elementFromPoint(pt, PT_DIR_EXACT);
    if (elem && elem->getRendMethod() == erm_final)
        return elem;
    return NULL;
}

/// returns rendering method
lvdom_element_render_method ldomNode::getRendMethod() {
    ASSERT_NODE_NOT_NULL;
    if (isElement()) {
        if (!isPersistent()) {
            return NPELEM->_rendMethod;
        } else {
            ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            return (lvdom_element_render_method)me->rendMethod;
        }
    }
    return erm_invisible;
}

/// sets rendering method
void ldomNode::setRendMethod(lvdom_element_render_method method) {
    ASSERT_NODE_NOT_NULL;
    if (isElement()) {
        if (!isPersistent()) {
            NPELEM->_rendMethod = method;
        } else {
            ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            if (me->rendMethod != method) {
                me->rendMethod = (lUInt8)method;
                modified();
            }
        }
    }
}

/// returns element style record
css_style_ref_t ldomNode::getStyle() const {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return css_style_ref_t();
    css_style_ref_t res = getDocument()->getNodeStyle(_handle._dataIndex);
    return res;
}

/// returns element font
font_ref_t ldomNode::getFont() {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return font_ref_t();
    return getDocument()->getNodeFont(_handle._dataIndex);
}

/// sets element font
void ldomNode::setFont(font_ref_t font) {
    ASSERT_NODE_NOT_NULL;
    if (isElement()) {
        getDocument()->setNodeFont(_handle._dataIndex, font);
    }
}

/// sets element style record
void ldomNode::setStyle(css_style_ref_t& style) {
    ASSERT_NODE_NOT_NULL;
    if (isElement()) {
        getDocument()->setNodeStyle(_handle._dataIndex, style);
    }
}

bool ldomNode::initNodeFont() {
    if (!isElement())
        return false;
    lUInt16 style = getDocument()->getNodeStyleIndex(_handle._dataIndex);
    lUInt16 font = getDocument()->getNodeFontIndex(_handle._dataIndex);
    lUInt16 fntIndex = getDocument()->_fontMap.get(style);
    if (fntIndex == 0) {
        css_style_ref_t s = getDocument()->_styles.get(style);
        if (s.isNull()) {
            CRLog::error("style not found for index %d", style);
            s = getDocument()->_styles.get(style);
        }
        LVFontRef fnt = ::getFont(this, s.get(), getDocument()->getFontContextDocIndex());
        fntIndex = (lUInt16)getDocument()->_fonts.cache(fnt);
        if (fnt.isNull()) {
            CRLog::error("font not found for style!");
            return false;
        } else {
            getDocument()->_fontMap.set(style, fntIndex);
        }
        if (font != 0) {
            if (font != fntIndex) // ???
                getDocument()->_fonts.release(font);
        }
        getDocument()->setNodeFontIndex(_handle._dataIndex, fntIndex);
        return true;
    } else {
        if (font != fntIndex)
            getDocument()->_fonts.addIndexRef(fntIndex);
    }
    if (fntIndex <= 0) {
        CRLog::error("font caching failed for style!");
        return false;
        ;
    } else {
        getDocument()->setNodeFontIndex(_handle._dataIndex, fntIndex);
    }
    return true;
}

void ldomNode::initNodeStyle() {
    // assume all parent styles already initialized
    if (!getDocument()->isDefStyleSet())
        return;
    if (isElement()) {
        if (isRoot() || getParentNode()->isRoot()) {
            setNodeStyle(this,
                         getDocument()->getDefaultStyle(),
                         getDocument()->getDefaultFont());
        } else {
            ldomNode* parent = getParentNode();

            // DEBUG TEST
            if (parent->getChildIndex(getDataIndex()) < 0) {
                CRLog::error("Invalid parent->child relation for nodes %d->%d", parent->getDataIndex(), getDataIndex());
            }

            //lvdomElementFormatRec * parent_fmt = node->getParentNode()->getRenderData();
            css_style_ref_t style = parent->getStyle();
            LVFontRef font = parent->getFont();
#if DEBUG_DOM_STORAGE == 1
            if (style.isNull()) {
                // for debugging
                CRLog::error("NULL style is returned for node <%s> %d level=%d  "
                             "parent <%s> %d level=%d children %d childIndex=%d",
                             LCSTR(getNodeName()), getDataIndex(), getNodeLevel(),
                             LCSTR(parent->getNodeName()), parent->getDataIndex(),
                             parent->getNodeLevel(), parent->getChildCount(), parent->getChildIndex(getDataIndex()));

                style = parent->getStyle();
            }
#endif
            setNodeStyle(this,
                         style,
                         font);
#if DEBUG_DOM_STORAGE == 1
            if (this->getStyle().isNull()) {
                CRLog::error("NULL style is set for <%s>", LCSTR(getNodeName()));
                style = this->getStyle();
            }
#endif
        }
    }
}

bool ldomNode::isBoxingNode(bool orPseudoElem, lUInt16 exceptBoxingNodeId) const {
    if (isElement()) {
        lUInt16 id = getNodeId();
        if (id >= EL_BOXING_START && id <= EL_BOXING_END && id != exceptBoxingNodeId) {
            return true;
        }
        if (orPseudoElem && id == el_pseudoElem) {
            return true;
        }
    }
    return false;
}

ldomNode* ldomNode::getUnboxedParent(lUInt16 exceptBoxingNodeId) const {
    ldomNode* parent = getParentNode();
    while (parent && parent->isBoxingNode(false, exceptBoxingNodeId))
        parent = parent->getParentNode();
    return parent;
}

// The following 4 methods are mostly used when checking CSS siblings/child
// rules and counting list items siblings: we have them skip pseudoElems by
// using isBoxingNode(orPseudoElem=true).
ldomNode* ldomNode::getUnboxedFirstChild(bool skip_text_nodes, lUInt16 exceptBoxingNodeId) const {
    for (int i = 0; i < getChildCount(); i++) {
        ldomNode* child = getChildNode(i);
        if (child && child->isBoxingNode(true, exceptBoxingNodeId)) {
            child = child->getUnboxedFirstChild(skip_text_nodes, exceptBoxingNodeId);
            // (child will then be NULL if it was a pseudoElem)
        }
        if (child && (!skip_text_nodes || !child->isText()))
            return child;
    }
    return NULL;
}

ldomNode* ldomNode::getUnboxedLastChild(bool skip_text_nodes, lUInt16 exceptBoxingNodeId) const {
    for (int i = getChildCount() - 1; i >= 0; i--) {
        ldomNode* child = getChildNode(i);
        if (child && child->isBoxingNode(true, exceptBoxingNodeId)) {
            child = child->getUnboxedLastChild(skip_text_nodes, exceptBoxingNodeId);
        }
        if (child && (!skip_text_nodes || !child->isText()))
            return child;
    }
    return NULL;
}

/* For reference, a non-recursive node subtree walker:
    ldomNode * n = topNode;
    if ( n && n->getChildCount() > 0 ) {
        int index = 0;
        n = n->getChildNode(index);
        while ( true ) {
            // Check the node only the first time we meet it (index == 0) and
            // not when we get back to it from a child to process next sibling
            if ( index == 0 ) {
                // Check n, process it, return it...
            }
            // Process next child
            if ( index < n->getChildCount() ) {
                n = n->getChildNode(index);
                index = 0;
                continue;
            }
            // No more child, get back to parent and have it process our sibling
            index = n->getNodeIndex() + 1;
            n = n->getParentNode();
            if ( n == topNode && index >= n->getChildCount() )
                break; // back to top node and all its children visited
        }
    }
*/

ldomNode* ldomNode::getUnboxedNextSibling(bool skip_text_nodes, lUInt16 exceptBoxingNodeId) const {
    // We use a variation of the above non-recursive node subtree walker,
    // but with an arbitrary starting node (this) inside the unboxed_parent
    // tree, and checks to not walk down non-boxing nodes - but still
    // walking up any node (which ought to be a boxing node).
    ldomNode* unboxed_parent = getUnboxedParent(exceptBoxingNodeId); // don't walk outside of it
    if (!unboxed_parent)
        return NULL;
    ldomNode* n = (ldomNode*)this;
    int index = 0;
    bool node_entered = true; // bootstrap loop
    // We may meet a same node as 'n' multiple times:
    // - once with node_entered=false and index being its real position inside
    //   its parent children collection, and we'll be "entering" it
    // - once with node_entered=true and index=0, meaning we have "entered" it to
    //   check if it's a candidate, and to possibly go on checking its own children.
    // - once when back from its children, with node_entered=false and index
    //   being that previous child index + 1, to go process its next sibling
    //   (or parent if no more sibling)
    while (true) {
        // printf("      %s\n", LCSTR(ldomXPointer(n,0).toStringV1()));
        if (node_entered && n != this) { // Don't check the starting node
            // Check if this node is a candidate
            if (n->isText()) { // Text nodes are not boxing nodes
                if (!skip_text_nodes)
                    return n;
            } else if (!n->isBoxingNode(true, exceptBoxingNodeId)) // Not a boxing node nor pseudoElem
                return n;
            // Otherwise, this node is a boxing node (or a text node or a pseudoElem
            // with no child, and we'll get back to its parent)
        }
        // Enter next node, and re-loop to have it checked
        // - if !node_entered : n is the parent and index points to the next child
        //   we want to check
        // - if n->isBoxingNode() (and node_entered=true, and index=0): enter the first
        //   child of this boxingNode (not if pseudoElem, that doesn't box anything)
        if ((!node_entered || n->isBoxingNode(false, exceptBoxingNodeId)) && index < n->getChildCount()) {
            n = n->getChildNode(index);
            index = 0;
            node_entered = true;
            continue;
        }
        // No more sibling/child to check, get back to parent and have it
        // process n's next sibling
        index = n->getNodeIndex() + 1;
        n = n->getParentNode();
        node_entered = false;
        if (n == unboxed_parent && index >= n->getChildCount()) {
            // back to real parent node and no more child to check
            break;
        }
    }
    return NULL;
}

ldomNode* ldomNode::getUnboxedPrevSibling(bool skip_text_nodes, lUInt16 exceptBoxingNodeId) const {
    // Similar to getUnboxedNextSibling(), but walking backward
    ldomNode* unboxed_parent = getUnboxedParent(exceptBoxingNodeId);
    if (!unboxed_parent)
        return NULL;
    ldomNode* n = (ldomNode*)this;
    int index = 0;
    bool node_entered = true; // bootstrap loop
    while (true) {
        // printf("      %s\n", LCSTR(ldomXPointer(n,0).toStringV1()));
        if (node_entered && n != this) {
            if (n->isText()) {
                if (!skip_text_nodes)
                    return n;
            } else if (!n->isBoxingNode(true, exceptBoxingNodeId))
                return n;
        }
        if ((!node_entered || n->isBoxingNode(false, exceptBoxingNodeId)) && index >= 0 && index < n->getChildCount()) {
            n = n->getChildNode(index);
            index = n->getChildCount() - 1;
            node_entered = true;
            continue;
        }
        index = n->getNodeIndex() - 1;
        n = n->getParentNode();
        node_entered = false;
        if (n == unboxed_parent && index < 0) {
            break;
        }
    }
    return NULL;
}

/// for display:list-item node, get marker
bool ldomNode::getNodeListMarker(int& counterValue, lString32& marker, int& markerWidth) {
    css_style_ref_t s = getStyle();
    marker.clear();
    markerWidth = 0;
    if (s.isNull())
        return false;
    css_list_style_type_t st = s->list_style_type;
    switch (st) {
        default:
            // treat default as disc
        case css_lst_disc:
            marker = U"\x2022"; // U"\x25CF" U"\x26AB" (medium circle) U"\x2981" (spot) U"\x2022" (bullet, small)
            break;
        case css_lst_circle:
            marker = U"\x25E6"; // U"\x25CB" U"\x26AA (medium) U"\25E6" (bullet) U"\x26AC (medium small)
            break;
        case css_lst_square:
            marker = U"\x25AA"; // U"\x25A0" U"\x25FE" (medium small) U"\x25AA" (small)
            break;
        case css_lst_none:
            // When css_lsp_inside, no space is used by the invisible marker
            if (s->list_style_position != css_lsp_inside) {
                marker = U"\x0020";
            }
            break;
        case css_lst_decimal:
        case css_lst_lower_roman:
        case css_lst_upper_roman:
        case css_lst_lower_alpha:
        case css_lst_upper_alpha:
            do {
                // If this element has a valid value then use it avoiding a walk.
                lString32 el_value = getAttributeValue(attr_value);
                if (!el_value.empty()) {
                    int el_ivalue;
                    if (el_value.atoi(el_ivalue)) {
                        counterValue = el_ivalue;
                        break;
                    }
                }

                // The UL > LI parent-child chain may have had some of our Boxing elements inserted
                ldomNode* parent = getUnboxedParent();

                // See if parent has a 'reversed' attribute.
                int increment = parent->hasAttribute(attr_reversed) ? -1 : +1;

                // If the caller passes in a non-zero counter then it is assumed
                // have been already calculated and have the value of the prior
                // element of a walk. There may be a redundant recalculation in
                // the case of zero.
                if (counterValue != 0) {
                    counterValue += increment;
                    break;
                }

                // See if parent has a valid 'start' attribute.
                // https://www.w3.org/TR/html5/grouping-content.html#the-ol-element
                // "The start attribute, if present, must be a valid integer giving the ordinal value of the first list item."
                lString32 start_value = parent->getAttributeValue(attr_start);
                int istart;
                if (!start_value.empty() && start_value.atoi(istart))
                    counterValue = istart;
                else if (increment > 0)
                    counterValue = 1;
                else {
                    // For a reversed ordering the default start is equal to the
                    // number of child elements.
                    counterValue = 0;

                    ldomNode* sibling = parent->getUnboxedFirstChild(true);
                    while (sibling) {
                        css_style_ref_t cs = sibling->getStyle();
                        if (cs.isNull()) { // Should not happen, but let's be sure
                            if (sibling == this)
                                break;
                            sibling = sibling->getUnboxedNextSibling(true);
                            continue;
                        }
                        if (cs->display != css_d_list_item_block && cs->display != css_d_list_item_legacy) {
                            // Alien element among list item nodes, skip it to not mess numbering
                            if (sibling == this) // Should not happen, but let's be sure
                                break;
                            sibling = sibling->getUnboxedNextSibling(true);
                            continue;
                        }
                        counterValue++;
                        sibling = sibling->getUnboxedNextSibling(true); // skip text nodes
                    }
                }

                // iterate parent's real children from start up to this node
                counterValue -= increment;
                ldomNode* sibling = parent->getUnboxedFirstChild(true);
                while (sibling) {
                    css_style_ref_t cs = sibling->getStyle();
                    if (cs.isNull()) { // Should not happen, but let's be sure
                        if (sibling == this)
                            break;
                        sibling = sibling->getUnboxedNextSibling(true);
                        continue;
                    }
                    if (cs->display != css_d_list_item_block && cs->display != css_d_list_item_legacy) {
                        // Alien element among list item nodes, skip it to not mess numbering
                        if (sibling == this) // Should not happen, but let's be sure
                            break;
                        sibling = sibling->getUnboxedNextSibling(true);
                        continue;
                    }

                    // Count advances irrespective of the list style.
                    counterValue += increment;

                    // See if it has a 'value' attribute that overrides the incremented value
                    // https://www.w3.org/TR/html5/grouping-content.html#the-li-element
                    // "The value attribute, if present, must be a valid integer giving the ordinal value of the list item."
                    lString32 value = sibling->getAttributeValue(attr_value);
                    if (!value.empty()) {
                        int ivalue;
                        if (value.atoi(ivalue))
                            counterValue = ivalue;
                    }
                    if (sibling == this)
                        break;
                    sibling = sibling->getUnboxedNextSibling(true); // skip text nodes
                }
            } while (0);

            static const char* lower_roman[] = { "i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix",
                                                 "x", "xi", "xii", "xiii", "xiv", "xv", "xvi", "xvii", "xviii", "xix",
                                                 "xx", "xxi", "xxii", "xxiii" };
            switch (st) {
                case css_lst_decimal:
                    marker = lString32::itoa(counterValue);
                    marker << '.';
                    break;
                case css_lst_lower_roman:
                    if (counterValue > 0 &&
                        counterValue - 1 < (int)(sizeof(lower_roman) / sizeof(lower_roman[0])))
                        marker = lString32(lower_roman[counterValue - 1]);
                    else
                        marker = lString32::itoa(counterValue); // fallback to simple counter
                    marker << '.';
                    break;
                case css_lst_upper_roman:
                    if (counterValue > 0 &&
                        counterValue - 1 < (int)(sizeof(lower_roman) / sizeof(lower_roman[0])))
                        marker = lString32(lower_roman[counterValue - 1]);
                    else
                        marker = lString32::itoa(counterValue); // fallback to simple digital counter
                    marker.uppercase();
                    marker << '.';
                    break;
                case css_lst_lower_alpha:
                    if (counterValue > 0 && counterValue <= 26)
                        marker.append(1, (lChar32)('a' + counterValue - 1));
                    else
                        marker = lString32::itoa(counterValue); // fallback to simple digital counter
                    marker << '.';
                    break;
                case css_lst_upper_alpha:
                    if (counterValue > 0 && counterValue <= 26)
                        marker.append(1, (lChar32)('A' + counterValue - 1));
                    else
                        marker = lString32::itoa(counterValue); // fallback to simple digital counter
                    marker << '.';
                    break;
                case css_lst_disc:
                case css_lst_circle:
                case css_lst_square:
                case css_lst_none:
                case css_lst_inherit:
                    // do nothing
                    break;
            }
            break;
    }
    bool res = false;
    if (!marker.empty()) {
        LVFontRef font = getFont();
        if (!font.isNull()) {
            TextLangCfg* lang_cfg = TextLangMan::getTextLangCfg(this);
            markerWidth = font->getTextWidth((marker + "  ").c_str(), marker.length() + 2, lang_cfg) + font->getSize() / 8;
            res = true;
        } else {
            marker.clear();
        }
    }
    return res;
}

/// returns first child node
ldomNode* ldomNode::getFirstChild() const {
    ASSERT_NODE_NOT_NULL;
    if (isElement()) {
        if (!isPersistent()) {
            tinyElement* me = NPELEM;
            if (me->_children.length())
                return getDocument()->getTinyNode(me->_children[0]);
        } else {
            ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            if (me->childCount)
                return getDocument()->getTinyNode(me->children[0]);
        }
    }
    return NULL;
}

/// returns last child node
ldomNode* ldomNode::getLastChild() const {
    ASSERT_NODE_NOT_NULL;
    if (isElement()) {
        if (!isPersistent()) {
            tinyElement* me = NPELEM;
            if (me->_children.length())
                return getDocument()->getTinyNode(me->_children[me->_children.length() - 1]);
        } else {
            ElementDataStorageItem* me = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            if (me->childCount)
                return getDocument()->getTinyNode(me->children[me->childCount - 1]);
        }
    }
    return NULL;
}

/// removes and deletes last child element
void ldomNode::removeLastChild() {
    ASSERT_NODE_NOT_NULL;
    if (hasChildren()) {
        ldomNode* lastChild = removeChild(getChildCount() - 1);
        lastChild->destroy();
    }
}

/// add child
void ldomNode::addChild(lInt32 childNodeIndex) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return;
    if (isPersistent())
        modify(); // convert to mutable element
    tinyElement* me = NPELEM;
    me->_children.add(childNodeIndex);
}

/// move range of children startChildIndex to endChildIndex inclusively to specified element
void ldomNode::moveItemsTo(ldomNode* destination, int startChildIndex, int endChildIndex) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return;
    if (isPersistent())
        modify();

#ifdef TRACE_AUTOBOX
    CRLog::debug("moveItemsTo() invoked from %d to %d", getDataIndex(), destination->getDataIndex());
#endif
    //if ( getDataIndex()==INDEX2 || getDataIndex()==INDEX1) {
    //    CRLog::trace("nodes from element %d are being moved", getDataIndex());
    //}
    /*#ifdef _DEBUG
    if ( !_document->checkConsistency( false ) )
        CRLog::error("before moveItemsTo");
#endif*/
    int len = endChildIndex - startChildIndex + 1;
    tinyElement* me = NPELEM;
    for (int i = 0; i < len; i++) {
        ldomNode* item = getChildNode(startChildIndex);
        //if ( item->getDataIndex()==INDEX2 || item->getDataIndex()==INDEX1 ) {
        //    CRLog::trace("node %d is being moved", item->getDataIndex() );
        //}
        me->_children.remove(startChildIndex); // + i
        item->setParentNode(destination);
        destination->addChild(item->getDataIndex());
    }
    destination->persist();
    // TODO: renumber rest of children in necessary
    /*#ifdef _DEBUG
    if ( !_document->checkConsistency( false ) )
        CRLog::error("after moveItemsTo");
#endif*/
}

/// find child element by tag id
ldomNode* ldomNode::findChildElement(lUInt16 nsid, lUInt16 id, int index) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return NULL;
    ldomNode* res = NULL;
    int k = 0;
    int childCount = getChildCount();
    for (int i = 0; i < childCount; i++) {
        ldomNode* p = getChildNode(i);
        if (!p->isElement())
            continue;
        if (p->getNodeId() == id && ((p->getNodeNsId() == nsid) || (nsid == LXML_NS_ANY))) {
            if (k == index || index == -1) {
                res = p;
                break;
            }
            k++;
        }
    }
    if (!res) //  || (index==-1 && k>1)  // DON'T CHECK WHETHER OTHER ELEMENTS EXIST
        return NULL;
    return res;
}

/// find child element by id path
ldomNode* ldomNode::findChildElement(lUInt16 idPath[]) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return NULL;
    ldomNode* elem = this;
    for (int i = 0; idPath[i]; i++) {
        elem = elem->findChildElement(LXML_NS_ANY, idPath[i], -1);
        if (!elem)
            return NULL;
    }
    return elem;
}

/// inserts child element
ldomNode* ldomNode::insertChildElement(lUInt32 index, lUInt16 nsid, lUInt16 id) {
    ASSERT_NODE_NOT_NULL;
    if (isElement()) {
        if (isPersistent())
            modify();
        tinyElement* me = NPELEM;
        if (index > (lUInt32)me->_children.length())
            index = me->_children.length();
        ldomNode* node = getDocument()->allocTinyElement(this, nsid, id);
        me->_children.insert(index, node->getDataIndex());
        return node;
    }
    readOnlyError();
    return NULL;
}

/// inserts child element
ldomNode* ldomNode::insertChildElement(lUInt16 id) {
    ASSERT_NODE_NOT_NULL;
    if (isElement()) {
        if (isPersistent())
            modify();
        ldomNode* node = getDocument()->allocTinyElement(this, LXML_NS_NONE, id);
        NPELEM->_children.insert(NPELEM->_children.length(), node->getDataIndex());
        return node;
    }
    readOnlyError();
    return NULL;
}

/// inserts child text
ldomNode* ldomNode::insertChildText(lUInt32 index, const lString32& value) {
    ASSERT_NODE_NOT_NULL;
    if (isElement()) {
        if (isPersistent())
            modify();
        tinyElement* me = NPELEM;
        if (index > (lUInt32)me->_children.length())
            index = me->_children.length();
#if !defined(USE_PERSISTENT_TEXT)
        ldomNode* node = getDocument()->allocTinyNode(NT_TEXT);
        lString8 s8 = UnicodeToUtf8(value);
        node->_data._text_ptr = new ldomTextNode(_handle._dataIndex, s8);
#else
        ldomNode* node = getDocument()->allocTinyNode(NT_PTEXT);
        //node->_data._ptext_addr._parentIndex = _handle._dataIndex;
        lString8 s8 = UnicodeToUtf8(value);
        node->_data._ptext_addr = getDocument()->_textStorage->allocText(node->_handle._dataIndex, _handle._dataIndex, s8);
#endif
        me->_children.insert(index, node->getDataIndex());
        return node;
    }
    readOnlyError();
    return NULL;
}

/// inserts child text
ldomNode* ldomNode::insertChildText(const lString32& value) {
    ASSERT_NODE_NOT_NULL;
    if (isElement()) {
        if (isPersistent())
            modify();
        tinyElement* me = NPELEM;
#if !defined(USE_PERSISTENT_TEXT)
        ldomNode* node = getDocument()->allocTinyNode(NT_TEXT);
        lString8 s8 = UnicodeToUtf8(value);
        node->_data._text_ptr = new ldomTextNode(_handle._dataIndex, s8);
#else
        ldomNode* node = getDocument()->allocTinyNode(NT_PTEXT);
        lString8 s8 = UnicodeToUtf8(value);
        node->_data._ptext_addr = getDocument()->_textStorage->allocText(node->_handle._dataIndex, _handle._dataIndex, s8);
#endif
        me->_children.insert(me->_children.length(), node->getDataIndex());
        return node;
    }
    readOnlyError();
    return NULL;
}

/// inserts child text
ldomNode* ldomNode::insertChildText(const lString8& s8, bool before_last_child) {
    ASSERT_NODE_NOT_NULL;
    if (isElement()) {
        if (isPersistent())
            modify();
        tinyElement* me = NPELEM;
#if !defined(USE_PERSISTENT_TEXT)
        ldomNode* node = getDocument()->allocTinyNode(NT_TEXT);
        node->_data._text_ptr = new ldomTextNode(_handle._dataIndex, s8);
#else
        ldomNode* node = getDocument()->allocTinyNode(NT_PTEXT);
        node->_data._ptext_addr = getDocument()->_textStorage->allocText(node->_handle._dataIndex, _handle._dataIndex, s8);
#endif
        int index = me->_children.length();
        if (before_last_child && index > 0)
            index--;
        me->_children.insert(index, node->getDataIndex());
        return node;
    }
    readOnlyError();
    return NULL;
}

/// remove child
ldomNode* ldomNode::removeChild(lUInt32 index) {
    ASSERT_NODE_NOT_NULL;
    if (isElement()) {
        if (isPersistent())
            modify();
        lUInt32 removedIndex = NPELEM->_children.remove(index);
        ldomNode* node = getTinyNode(removedIndex);
        return node;
    }
    readOnlyError();
    return NULL;
}

/// creates stream to read base64 encoded data from element
LVStreamRef ldomNode::createBase64Stream() {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return LVStreamRef();
#define DEBUG_BASE64_IMAGE 0
#if DEBUG_BASE64_IMAGE == 1
    lString32 fname = getAttributeValue(attr_id);
    lString8 fname8 = UnicodeToUtf8(fname);
    LVStreamRef ostream = LVOpenFileStream(fname.empty() ? U"image.png" : fname.c_str(), LVOM_WRITE);
    printf("createBase64Stream(%s)\n", fname8.c_str());
#endif
    LVStream* stream = new LVBase64NodeStream(this);
    if (stream->GetSize() == 0) {
#if DEBUG_BASE64_IMAGE == 1
        printf("    cannot create base64 decoder stream!!!\n");
#endif
        delete stream;
        return LVStreamRef();
    }
    LVStreamRef istream(stream);

#if DEBUG_BASE64_IMAGE == 1
    LVPumpStream(ostream, istream);
    istream->SetPos(0);
#endif

    return istream;
}

/// returns object image ref name
lString32 ldomNode::getObjectImageRefName(bool percentDecode) {
    if (!isElement())
        return lString32::empty_str;
    //printf("ldomElement::getObjectImageSource() ... ");
    const css_elem_def_props_t* et = getDocument()->getElementTypePtr(getNodeId());
    if (!et || !et->is_object)
        return lString32::empty_str;
    lUInt16 hrefId = getDocument()->getAttrNameIndex("href");
    lUInt16 srcId = getDocument()->getAttrNameIndex("src");
    lUInt16 recIndexId = getDocument()->getAttrNameIndex("recindex");
    lString32 refName = getAttributeValue(getDocument()->getNsNameIndex("xlink"),
                                          hrefId);

    if (refName.empty())
        refName = getAttributeValue(getDocument()->getNsNameIndex("l"), hrefId);
    if (refName.empty())
        refName = getAttributeValue(LXML_NS_ANY, hrefId); //LXML_NS_NONE
    if (refName.empty())
        refName = getAttributeValue(LXML_NS_ANY, srcId); //LXML_NS_NONE
    if (refName.empty()) {
        lString32 recindex = getAttributeValue(LXML_NS_ANY, recIndexId);
        if (!recindex.empty()) {
            int n;
            if (recindex.atoi(n)) {
                refName = lString32(MOBI_IMAGE_NAME_PREFIX) + fmt::decimal(n);
                //CRLog::debug("get mobi image %s", LCSTR(refName));
            }
        }
        //        else {
        //            for (int k=0; k<getAttrCount(); k++) {
        //                CRLog::debug("attr %s=%s", LCSTR(getAttributeName(k)), LCSTR(getAttributeValue(getAttributeName(k).c_str())));
        //            }
        //        }
    }
    if (refName.length() < 2)
        return lString32::empty_str;
    if (percentDecode)
        refName = DecodeHTMLUrlString(refName);
    return refName;
}

/// returns object image stream
LVStreamRef ldomNode::getObjectImageStream() {
    lString32 refName = getObjectImageRefName();
    if (refName.empty())
        return LVStreamRef();
    return getDocument()->getObjectImageStream(refName);
}

/// returns object image source
LVImageSourceRef ldomNode::getObjectImageSource() {
    lString32 refName = getObjectImageRefName(true);
    LVImageSourceRef ref;
    if (refName.empty())
        return ref;
    ref = getDocument()->getObjectImageSource(refName);
    if (ref.isNull()) {
        // try again without percent decoding (for fb3)
        refName = getObjectImageRefName(false);
        if (refName.empty())
            return ref;
        ref = getDocument()->getObjectImageSource(refName);
    }
    if (!ref.isNull()) {
        int dx = ref->GetWidth();
        int dy = ref->GetHeight();
        ref = LVImageSourceRef(new NodeImageProxy(this, refName, dx, dy));
    } else {
        CRLog::error("ObjectImageSource cannot be opened by name %s", LCSTR(refName));
    }

    getDocument()->_urlImageMap.set(refName, ref);
    return ref;
}

/// returns the sum of this node and its parents' top and bottom margins, borders and paddings
int ldomNode::getSurroundingAddedHeight() {
    int h = 0;
    ldomNode* n = this;
    while (true) {
        ldomNode* parent = n->getParentNode();
        lvdom_element_render_method rm = n->getRendMethod();
        if (rm != erm_inline && rm != erm_invisible && rm != erm_killed) {
            // Add offset of border and padding
            int base_width = 0;
            if (parent && !(parent->isNull())) {
                // margins and padding in % are scaled according to parent's width
                RenderRectAccessor fmt(parent);
                base_width = fmt.getWidth();
            }
            css_style_ref_t style = n->getStyle();
            h += lengthToPx(n, style->margin[2], base_width);  // top margin
            h += lengthToPx(n, style->margin[3], base_width);  // bottom margin
            h += lengthToPx(n, style->padding[2], base_width); // top padding
            h += lengthToPx(n, style->padding[3], base_width); // bottom padding
            h += measureBorder(n, 0);                          // top border
            h += measureBorder(n, 2);                          // bottom border
        }
        if (!parent || parent->isNull())
            break;
        n = parent;
    }
    return h;
}

/// formats final block
// 'fmt' is the rect of the block node, and MUST have its width set
// (as ::renderFinalBlock( this, f.get(), fmt...) needs it to compute text-indent in %
// 'int width' is the available width for the inner content, and so
// caller must exclude block node padding from it.
int ldomNode::renderFinalBlock(LFormattedTextRef& frmtext, RenderRectAccessor* fmt, int width, BlockFloatFootprint* float_footprint) {
    ASSERT_NODE_NOT_NULL;
    if (!isElement())
        return 0;
    //CRLog::trace("renderFinalBlock()");
    CVRendBlockCache& cache = getDocument()->getRendBlockCache();
    LFormattedTextRef f;
    lvdom_element_render_method rm = getRendMethod();

    if (cache.get(this, f)) {
        if (f->isReusable()) {
            frmtext = f;
            if (rm != erm_final)
                return 0;
            //RenderRectAccessor fmt( this );
            //CRLog::trace("Found existing formatted object for node #%08X", (lUInt32)this);
            return fmt->getHeight();
        }
        // Not resuable: remove it, just to be sure it's properly freed
        cache.remove(this);
    }
    f = getDocument()->createFormattedText();
    if (rm != erm_final)
        return 0;

    /// Render whole node content as single formatted object

    // Get some properties cached in this node's RenderRectAccessor
    // and set the initial flags and lang_cfg (for/from the final node
    // itself) for renderFinalBlock(),
    int direction = RENDER_RECT_PTR_GET_DIRECTION(fmt);
    lUInt32 flags = styleToTextFmtFlags(true, getStyle(), 0, direction);
    int lang_node_idx = fmt->getLangNodeIndex();
    TextLangCfg* lang_cfg = TextLangMan::getTextLangCfg(lang_node_idx > 0 ? getDocument()->getTinyNode(lang_node_idx) : NULL);

    // Add this node's inner content (text and children nodes) as source text
    // and image fragments into the empty LFormattedText object
    ::renderFinalBlock(this, f.get(), fmt, flags, 0, -1, lang_cfg);
    // We need to store this LFormattedTextRef in the cache for it to
    // survive when leaving this function (some callers do use it).
    cache.set(this, f);

    // Gather some outer properties and context, so we can format (render)
    // the inner content in that context.
    // This page_h we provide to f->Format() is only used to enforce a max height to images
    int page_h = getDocument()->getPageHeight();
    // Save or restore outer floats footprint (it is only provided
    // when rendering the document - when this is called to draw the
    // node, or search for text and links, we need to get it from
    // the cached RenderRectAccessor).
    BlockFloatFootprint restored_float_footprint; // (need to be available when we exit the else {})
    if (float_footprint) {                        // Save it in this node's RenderRectAccessor
        float_footprint->store(this);
    } else { // Restore it from this node's RenderRectAccessor
        float_footprint = &restored_float_footprint;
        float_footprint->restore(this, (lUInt16)width);
    }
    if (!getDocument()->isRendered()) {
        // Full rendering in progress: avoid some uneeded work that
        // is only needed when we'll be drawing the formatted text
        // (like alignLign()): this will mark it as not reusable, and
        // one that is on a page to be drawn will be reformatted .
        f->requestLightFormatting();
    }
    int usable_left_overflow = fmt->getUsableLeftOverflow();
    int usable_right_overflow = fmt->getUsableRightOverflow();

    // Note: some properties are set into LFormattedText by lvrend.cpp's renderFinalBlock(),
    // while some others are only passed below as parameters to LFormattedText->Format().
    // The former should logically be source inner content properties (strut, text indent)
    // while the latter should be formatting and outer context properties (block width,
    // page height...).
    // There might be a few drifts from that logic, or duplicates ('direction' is
    // passed both ways), that could need a little rework.

    // Format/render inner content: this makes lines and words, which are
    // cached into the LFormattedText and ready to be used for drawing
    // and text selection.
    int h = f->Format((lUInt16)width, (lUInt16)page_h, direction, usable_left_overflow, usable_right_overflow,
                      getDocument()->getHangingPunctiationEnabled(), float_footprint);
    frmtext = f;
    //CRLog::trace("Created new formatted object for node #%08X", (lUInt32)this);
    return h;
}

/// formats final block again after change, returns true if size of block is changed
/// (not used anywhere, not updated to use RENDER_RECT_HAS_FLAG(fmt, INNER_FIELDS_SET)
bool ldomNode::refreshFinalBlock() {
    ASSERT_NODE_NOT_NULL;
    if (getRendMethod() != erm_final)
        return false;
    // TODO: implement reformatting of one node
    CVRendBlockCache& cache = getDocument()->getRendBlockCache();
    cache.remove(this);
    RenderRectAccessor fmt(this);
    lvRect oldRect, newRect;
    fmt.getRect(oldRect);
    LFormattedTextRef txtform;
    int width = fmt.getWidth();
    renderFinalBlock(txtform, &fmt, width - measureBorder(this, 1) - measureBorder(this, 3) - lengthToPx(this, this->getStyle()->padding[0], fmt.getWidth()) - lengthToPx(this, this->getStyle()->padding[1], fmt.getWidth()));
    fmt.getRect(newRect);
    if (oldRect == newRect)
        return false;
    // TODO: relocate other blocks
    return true;
}

/// replace node with r/o persistent implementation
ldomNode* ldomNode::persist() {
    ASSERT_NODE_NOT_NULL;
    if (!isPersistent()) {
        if (isElement()) {
            // ELEM->PELEM
            tinyElement* elem = NPELEM;
            int attrCount = elem->_attrs.length();
            int childCount = elem->_children.length();
            _handle._dataIndex = (_handle._dataIndex & ~0xF) | NT_PELEMENT;
            _data._pelem_addr = getDocument()->_elemStorage->allocElem(_handle._dataIndex, elem->_parentNode ? elem->_parentNode->_handle._dataIndex : 0, elem->_children.length(), elem->_attrs.length());
            ElementDataStorageItem* data = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            data->nsid = elem->_nsid;
            data->id = elem->_id;
            lUInt16* attrs = data->attrs();
            int i;
            for (i = 0; i < attrCount; i++) {
                const lxmlAttribute* attr = elem->_attrs[i];
                attrs[i * 4] = attr->nsid;                          // namespace
                attrs[i * 4 + 1] = attr->id;                        // id
                attrs[i * 4 + 2] = (lUInt16)(attr->index & 0xFFFF); // value lower 2-bytes
                attrs[i * 4 + 3] = (lUInt16)(attr->index >> 16);    // value higher 2-bytes
            }
            for (i = 0; i < childCount; i++) {
                data->children[i] = elem->_children[i];
            }
            data->rendMethod = (lUInt8)elem->_rendMethod;
            delete elem;
        } else {
            // TEXT->PTEXT
            lString8 utf8 = _data._text_ptr->getText();
            lUInt32 parentIndex = _data._text_ptr->getParentIndex();
            delete _data._text_ptr;
            _handle._dataIndex = (_handle._dataIndex & ~0xF) | NT_PTEXT;
            _data._ptext_addr = getDocument()->_textStorage->allocText(_handle._dataIndex, parentIndex, utf8);
            // change type
        }
    }
    return this;
}

/// replace node with r/w implementation
ldomNode* ldomNode::modify() {
    ASSERT_NODE_NOT_NULL;
    if (isPersistent()) {
        if (isElement()) {
            // PELEM->ELEM
            ElementDataStorageItem* data = getDocument()->_elemStorage->getElem(_data._pelem_addr);
            tinyElement* elem = new tinyElement(getDocument(), getParentNode(), data->nsid, data->id);
            for (int i = 0; i < data->childCount; i++)
                elem->_children.add(data->children[i]);
            for (int i = 0; i < data->attrCount; i++)
                elem->_attrs.add(data->attr(i));
            _handle._dataIndex = (_handle._dataIndex & ~0xF) | NT_ELEMENT;
            elem->_rendMethod = (lvdom_element_render_method)data->rendMethod;
            getDocument()->_elemStorage->freeNode(_data._pelem_addr);
            NPELEM = elem;
        } else {
            // PTEXT->TEXT
            // convert persistent text to mutable
            lString8 utf8 = getDocument()->_textStorage->getText(_data._ptext_addr);
            lUInt32 parentIndex = getDocument()->_textStorage->getParent(_data._ptext_addr);
            getDocument()->_textStorage->freeNode(_data._ptext_addr);
            _data._text_ptr = new ldomTextNode(parentIndex, utf8);
            // change type
            _handle._dataIndex = (_handle._dataIndex & ~0xF) | NT_TEXT;
        }
    }
    return this;
}
