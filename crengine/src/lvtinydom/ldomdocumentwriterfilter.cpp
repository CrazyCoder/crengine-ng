/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2008-2012,2014 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2020,2021 Aleksey Chernov <valexlin@gmail.com>          *
 *   Copyright (C) 2018-2021 poire-z <poire-z@users.noreply.github.com>    *
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

#include "ldomdocumentwriterfilter.h"

#include <ldomdocument.h>
#include <lvdocprops.h>
#include <fb2def.h>
#include <lvstreamutils.h>
#include <crlog.h>

#include "ldomelementwriter.h"
#include "../lvxml/lvfileformatparser.h"
#include "../lvxml/lvxmlutils.h"

/////////////////////////////////////////////////////////////////
/// ldomDocumentWriterFilter
// Used to parse lousy HTML in formats: HTML, CHM, PDB(html)
// For all these document formats, it is fed by HTMLParser that does
// convert to lowercase the tag names and attributes.
// ldomDocumentWriterFilter does then deal with auto-closing unbalanced
// HTML tags according to the rules set in crengine/src/lvxml.cpp HTML_AUTOCLOSE_TABLE[]

/**
 * \brief callback object to fill DOM tree
 * To be used with XML parser as callback object.
 * Creates document according to incoming events.
 * Autoclose HTML tags.
 */
void ldomDocumentWriterFilter::setClass(const lChar32* className, bool overrideExisting) {
    ldomNode* node = _currNode->_element;
    if (_classAttrId == 0) {
        _classAttrId = _document->getAttrNameIndex(U"class");
    }
    if (overrideExisting || !node->hasAttribute(_classAttrId)) {
        node->setAttributeValue(LXML_NS_NONE, _classAttrId, className);
    }
}

void ldomDocumentWriterFilter::appendStyle(const lChar32* style) {
    ldomNode* node = _currNode->_element;
    if (_styleAttrId == 0) {
        _styleAttrId = _document->getAttrNameIndex(U"style");
    }
    // Append to the style attribute even if embedded styles are disabled
    // at loading time, otherwise it won't be there if we enable them later
    // if (!_document->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES))
    //     return; // disabled

    lString32 oldStyle = node->getAttributeValue(_styleAttrId);
    if (!oldStyle.empty() && oldStyle.at(oldStyle.length() - 1) != ';')
        oldStyle << "; ";
    oldStyle << style;
    node->setAttributeValue(LXML_NS_NONE, _styleAttrId, oldStyle.c_str());
}

// Legacy auto close handler (gDOMVersionRequested < 20200824)
void ldomDocumentWriterFilter::AutoClose(lUInt16 tag_id, bool open) {
    lUInt16* rule = _rules[tag_id];
    if (!rule)
        return;
    if (open) {
        ldomElementWriter* found = NULL;
        ldomElementWriter* p = _currNode;
        while (p && !found) {
            lUInt16 id = p->_element->getNodeId();
            for (int i = 0; rule[i]; i++) {
                if (rule[i] == id) {
                    found = p;
                    break;
                }
            }
            p = p->_parent;
        }
        // found auto-close target
        if (found != NULL) {
            bool done = false;
            while (!done && _currNode) {
                if (_currNode == found)
                    done = true;
                ldomNode* closedElement = _currNode->getElement();
                _currNode = pop(_currNode, closedElement->getNodeId());
                //ElementCloseHandler( closedElement );
            }
        }
    } else {
        if (!rule[0])
            _currNode = pop(_currNode, _currNode->getElement()->getNodeId());
    }
}

// With gDOMVersionRequested >= 20200824, we use hardcoded rules
// for opening and closing tags, trying to follow what's relevant
// in the HTML Living Standard (=HTML5):
//   https://html.spec.whatwg.org/multipage/parsing.html
// A less frightening introduction is available at:
//   https://htmlparser.info/parser/
//
// Note that a lot of rules and checks in the algorithm are for
// noticing "parser errors", with usually a fallback of ignoring
// it and going on.
// We ensure one tedious requirement: foster parenting of non-table
// elements met while building a table, mostly to not have mis-nested
// content simply ignored and not shown to the user.
// Other tedious requirements not ensured might just have some impact
// on the styling of the content, which should be a minor issue.
//
// It feels that we can simplify it to the following implementation,
// with possibly some cases not handled related to:
// - FORM and form elements (SELECT, INPUT, OPTION...)
// - TEMPLATE, APPLET, OBJECT, MARQUEE
// - Mis-nested HTML/BODY/HEAD
// - Reconstructing the active formatting elements (B, I...) when
//   mis-nested or "on hold" when entering block or table elements.
// - The "adoption agency algorithm" for mis-nested formatting
//   elements (and nested <A>)
// - We may not ignore some opening tag that we normally should
//   (like HEAD or FRAME when in BODY) (but we ignore a standalone
//   sub-table element when not inside a TABLE) as this would
//   complicate the internal parser state.
//
// Of interest:
// https://html.spec.whatwg.org/multipage/parsing.html#parse-state
//   List of "special" elements
//   List of elements for rules "have a particular element in X scope"
// https://html.spec.whatwg.org/multipage/parsing.html#tree-construction
//   Specific rules when start or end tag of specific elements is met

// Scope are for limiting ancestor search when looking for a previous
// element to close (a closing tag may be ignored if no opening tag is
// found in the specified scope)
enum ScopeType
{
    HTML_SCOPE_NONE = 0,  // no stop tag
    HTML_SCOPE_MAIN,      // HTML, TABLE, TD, TH, CAPTION, APPLET, MARQUEE, OBJECT, TEMPLATE
    HTML_SCOPE_LIST_ITEM, // = SCOPE_MAIN + OL, UL
    HTML_SCOPE_BUTTON,    // = SCOPE_MAIN + BUTTON (not used, only used with P that we handle specifically)
    HTML_SCOPE_TABLE,     // HTML, TABLE, TEMPLATE
    HTML_SCOPE_SELECT,    // All elements stop, except OPTGROUP, OPTION
    HTML_SCOPE_SPECIALS,  // All specials elements (inline don't close across block/specials elements)
    // Next ones are scopes with specific behaviours that may ignore target_id
    HTML_SCOPE_OPENING_LI,          // = SCOPE_SPECIALS, minus ADDRESS, DIV, P: close any LI
    HTML_SCOPE_OPENING_DT_DD,       // = SCOPE_SPECIALS, minus ADDRESS, DIV, P: close any DT/DD
    HTML_SCOPE_OPENING_H1_H6,       // = close current node if H1, H2, H3, H4, H5, H6
    HTML_SCOPE_CLOSING_H1_H6,       // = SCOPE_MAIN: close any of H1..H6
    HTML_SCOPE_TABLE_TO_TOP,        // = SCOPE_TABLE: close all table sub-elements to end up being TABLE
    HTML_SCOPE_TABLE_OPENING_TD_TH, // = SCOPE_TABLE: close any TD/TH
};
// Note: as many elements close a P, we don't handle checking and closing them
// via popUpTo(NULL, el_p, HTML_SCOPE_BUTTON), but we keep the last P as _lastP
// so we can just popUpTo(_lastP) if set when meeting a "close a P" element.

// Boxing elements (id < el_DocFragment) (and DocFragment itself,
// not used with the HTMLParser) are normally added by crengine
// after "delete ldomElementWriter" (which calls onBodyExit()
// which calls initNodeRendMethod()), so after we have closed
// and pass by the element.
// So, we shouldn't meet any in popUpTo() and don't have to wonder
// if we should stop at them, or pass by them.
// Except <mathBox> that might be added when parsing MathML,
// and so can be met when poping up nodes.

lUInt16 ldomDocumentWriterFilter::popUpTo(ldomElementWriter* target, lUInt16 target_id, int scope) {
    if (!target) {
        // Check if there's an element with provided target_id in the stack inside this scope
        ldomElementWriter* tmp = _currNode;
        while (tmp) {
            lUInt16 tmpId = tmp->getElement()->getNodeId();
            if (tmpId < el_DocFragment && tmpId > el_NULL && tmpId != el_mathBox) {
                // We shouldn't meet any (see comment above), except mathBox
                // (but we can meet the root node when poping </html>)
                crFatalError(127, "Unexpected boxing element met in ldomDocumentWriterFilter::popUpTo()");
            }
            if (target_id && tmpId == target_id)
                break;
            if (_curFosteredNode && tmp == _curFosteredNode) {
                // If fostering and we're not closing the fostered node itself,
                // don't go at closing stuff above the fostered node
                tmp = NULL;
                break;
            }
            // Check scope stop tags
            bool stop = false;
            switch (scope) {
                case HTML_SCOPE_MAIN: // stop at HTML/TABLE/TD...
                    if (tmpId == el_html || tmpId == el_table || tmpId == el_td || tmpId == el_th || tmpId == el_caption ||
                        tmpId == el_applet || tmpId == el_marquee || tmpId == el_object || tmpId == el_template) {
                        tmp = NULL;
                        stop = true;
                    }
                    break;
                case HTML_SCOPE_LIST_ITEM: // stop at SCOPE_MAIN + OL, UL
                    if (tmpId == el_html || tmpId == el_table || tmpId == el_td || tmpId == el_th || tmpId == el_caption ||
                        tmpId == el_applet || tmpId == el_marquee || tmpId == el_object || tmpId == el_template ||
                        tmpId == el_ol || tmpId == el_ul) {
                        tmp = NULL;
                        stop = true;
                    }
                    break;
                case HTML_SCOPE_BUTTON: // stop at SCOPE_MAIN + BUTTON
                    if (tmpId == el_html || tmpId == el_table || tmpId == el_td || tmpId == el_th || tmpId == el_caption ||
                        tmpId == el_applet || tmpId == el_marquee || tmpId == el_object || tmpId == el_template ||
                        tmpId == el_button) {
                        tmp = NULL;
                        stop = true;
                    }
                    break;
                case HTML_SCOPE_TABLE: // stop at HTML and TABLE
                    if (tmpId == el_html || tmpId == el_table || tmpId == el_template) {
                        tmp = NULL;
                        stop = true;
                    }
                    break;
                case HTML_SCOPE_SELECT:
                    // This one is different: all elements stop it, except optgroup and option
                    if (tmpId != el_optgroup && tmpId != el_option) {
                        tmp = NULL;
                        stop = true;
                    }
                    break;
                case HTML_SCOPE_SPECIALS: // stop at any "special" element
                    if (tmpId >= EL_SPECIAL_START && tmpId <= EL_SPECIAL_END) {
                        tmp = NULL;
                        stop = true;
                    }
                    break;
                case HTML_SCOPE_OPENING_LI:
                    if (tmpId == el_li) {
                        stop = true;
                    } else if (tmpId >= EL_SPECIAL_START && tmpId <= EL_SPECIAL_END &&
                               tmpId != el_div && tmpId != el_p && tmpId != el_address) {
                        tmp = NULL;
                        stop = true;
                    }
                    break;
                case HTML_SCOPE_OPENING_DT_DD:
                    if (tmpId == el_dt || tmpId == el_dd) {
                        stop = true;
                    } else if (tmpId >= EL_SPECIAL_START && tmpId <= EL_SPECIAL_END &&
                               tmpId != el_div && tmpId != el_p && tmpId != el_address) {
                        tmp = NULL;
                        stop = true;
                    }
                    break;
                case HTML_SCOPE_OPENING_H1_H6:
                    // Close immediate parent H1...H6, but don't walk up
                    // <H3> ... <H4> : H4 will close H3
                    // <H3> ... <B> ... <H4> : H4 will not close H3
                    if (tmpId < el_h1 || tmpId > el_h6) {
                        tmp = NULL; // Nothing to close
                    }
                    stop = true; // Don't check upper
                    break;
                case HTML_SCOPE_CLOSING_H1_H6:
                    if (tmpId >= el_h1 && tmpId <= el_h6) {
                        stop = true;
                    } else if (tmpId == el_html || tmpId == el_table || tmpId == el_td || tmpId == el_th || tmpId == el_caption ||
                               tmpId == el_applet || tmpId == el_marquee || tmpId == el_object || tmpId == el_template) {
                        tmp = NULL;
                        stop = true;
                    }
                    break;
                case HTML_SCOPE_TABLE_TO_TOP:
                    if (tmp->_parent && tmp->_parent->getElement()->getNodeId() == el_table) {
                        stop = true;
                    } else if (tmpId == el_html || tmpId == el_table || tmpId == el_template) {
                        tmp = NULL;
                        stop = true;
                    }
                    break;
                case HTML_SCOPE_TABLE_OPENING_TD_TH:
                    if (tmpId == el_td || tmpId == el_th) {
                        stop = true;
                    } else if (tmpId == el_html || tmpId == el_table || tmpId == el_template) {
                        tmp = NULL;
                        stop = true;
                    }
                    break;
                case HTML_SCOPE_NONE:
                default:
                    // Never stop, continue up to root node
                    break;
            }
            if (stop)
                break;
            tmp = tmp->_parent;
        }
        target = tmp; // (NULL if not found, NULL or not if stopped)
    }
    if (target) {
        // Assume target is valid and will be found
        while (_currNode) {
            // Update state for after this node is closed
            lUInt16 curNodeId = _currNode->getElement()->getNodeId();
            // Reset these flags if we see again these tags (so to
            // at least reconstruct </html><html><body><hr> when
            // meeting </html><hr> and have el_html as the catch all
            // element in SCOPEs working.
            if (curNodeId == el_body) {
                _bodyTagSeen = false;
            } else if (curNodeId == el_html) {
                _headTagSeen = false;
                _htmlTagSeen = false;
            }
            if (_lastP && _currNode == _lastP)
                _lastP = NULL;
            ldomElementWriter* tmp = _currNode;
            bool done = _currNode == target;
            if (_curFosteredNode && _currNode == _curFosteredNode) {
                // If we meet the fostered node, have it closed but don't
                // go at closing above it
                done = true;
                _currNode = _curNodeBeforeFostering;
                _curNodeBeforeFostering = NULL;
                _curFosteredNode = NULL;
            } else {
                _currNode = _currNode->_parent;
            }
            ElementCloseHandler(tmp->getElement());
            delete tmp;
            if (done)
                break;
        }
    }
    return _currNode ? _currNode->getElement()->getNodeId() : el_NULL;
}

// To give as first parameter to AutoOpenClosePop()
enum ParserStepType
{
    PARSER_STEP_TAG_OPENING = 1,
    PARSER_STEP_TAG_CLOSING,
    PARSER_STEP_TAG_SELF_CLOSING,
    PARSER_STEP_TEXT
};

// More HTML5 conforming auto close handler (gDOMVersionRequested >= 20200824)
bool ldomDocumentWriterFilter::AutoOpenClosePop(int step, lUInt16 tag_id) {
    lUInt16 curNodeId = _currNode ? _currNode->getElement()->getNodeId() : el_NULL;
    if (!_bodyTagSeen && (step == PARSER_STEP_TAG_OPENING || step == PARSER_STEP_TEXT)) {
        // Create some expected containing elements if not yet seen
        if (!_headTagSeen) {
            if (!_htmlTagSeen) {
                _htmlTagSeen = true;
                if (tag_id != el_html) {
                    OnTagOpen(U"", U"html");
                    OnTagBody();
                }
            }
            if ((tag_id >= EL_IN_HEAD_START && tag_id <= EL_IN_HEAD_END) || tag_id == el_noscript) {
                _headTagSeen = true;
                if (tag_id != el_head) {
                    OnTagOpen(U"", U"head");
                    OnTagBody();
                }
            }
            curNodeId = _currNode ? _currNode->getElement()->getNodeId() : el_NULL;
        }
        if (tag_id >= EL_IN_BODY_START || (step == PARSER_STEP_TEXT && (curNodeId == el_html || curNodeId == el_head))) {
            // Tag usually found inside <body>, or text while being <HTML> or <HEAD>
            // (text while being in <HTML><HEAD><TITLE> should not trigger this):
            // end of <head> and start of <body>
            if (_headTagSeen)
                OnTagClose(U"", U"head");
            else
                _headTagSeen = true; // We won't open any <head> anymore
            _bodyTagSeen = true;
            if (tag_id != el_body) {
                OnTagOpen(U"", U"body");
                OnTagBody();
            }
            curNodeId = _currNode ? _currNode->getElement()->getNodeId() : el_NULL;
        }
    }
    if (step == PARSER_STEP_TEXT) // new text: nothing more to do
        return true;

    bool is_self_closing_tag = false;
    switch (tag_id) {
        // These are scaterred among different ranges, so we sadly
        // can't use any range comparisons
        case el_area:
        case el_base:
        case el_br:
        case el_col:
        case el_embed:
        case el_hr:
        case el_img:
        case el_input:
        case el_link:
        case el_meta:
        case el_param:
        case el_source:
        case el_track:
        case el_wbr:
            is_self_closing_tag = true;
            break;
        default:
            break;
    }

    if (step == PARSER_STEP_TAG_OPENING) {
        // A new element with tag_id will be created after we return
        // We should
        // - create implicit parent elements for tag_id if not present (partially
        //   done for HTML/HEAD/BODY elements above)
        // - close elements that should be closed by this tag_id (some with optional
        //   end tags and others that the spec says so)
        // - keep a note if it's self-closing so we can close it when appropriate
        // - ignore this opening tag in some cases

        // Table elements can be ignored, create missing elements
        // and/or close some others
        if (tag_id == el_th || tag_id == el_td) {
            // Close any previous TD/TH in table scope if any
            curNodeId = popUpTo(NULL, 0, HTML_SCOPE_TABLE_OPENING_TD_TH);
            // We should be in a table or sub-table element
            // (a standalone TD is ignored)
            if (curNodeId < el_table || curNodeId > el_tr)
                return false; // Not in a table context: ignore this TD/TH
            // We must be in a TR. If we're not, have missing elements created
            if (curNodeId != el_tr) {
                // This will create all the other missing elements if needed
                OnTagOpen(U"", U"tr");
                OnTagBody();
            }
        } else if (tag_id == el_tr) {
            // Close any previous TR in table scope if any
            curNodeId = popUpTo(NULL, tag_id, HTML_SCOPE_TABLE);
            // We should be in a table or sub-table element
            // (a standalone TR is ignored)
            if (curNodeId < el_table || curNodeId > el_tfoot)
                return false; // Not in a table context: ignore this TR
            // We must be in a THEAD/TBODY/TFOOT. If we're not, have missing elements created
            if (curNodeId < el_thead || curNodeId > el_tfoot) {
                // This will create all the other missing elements if needed
                OnTagOpen(U"", U"tbody");
                OnTagBody();
            }
        } else if (tag_id == el_col) {
            // Close any previous COL in table scope if any
            curNodeId = popUpTo(NULL, tag_id, HTML_SCOPE_TABLE);
            // We should be in a table or sub-table element
            if (curNodeId < el_table || curNodeId > el_td)
                return false; // Not in a table context: ignore this TR
            // We must be in a COLGROUP. If we're not, have missing elements created
            if (curNodeId != el_colgroup) {
                // This will create all the other missing elements if needed
                OnTagOpen(U"", U"colgroup");
                OnTagBody();
            }
        } else if ((tag_id >= el_thead && tag_id <= el_tfoot) ||
                   tag_id == el_caption ||
                   tag_id == el_colgroup) {
            // Close any previous THEAD/TBODY/TFOOT/CAPTION/COLGROUP/COL in table scope if any
            curNodeId = popUpTo(NULL, 0, HTML_SCOPE_TABLE_TO_TOP);
            // We should be in a table element
            if (curNodeId != el_table)
                return false; // Not in a table context
        }

        if (tag_id == el_li) {
            // A LI should close any previous LI, but should stop at specials
            // except ADDRESS, DIV and P (they will so stop at UL/OL and won't
            // close any upper LI that had another level of list opened).
            // Once that LI close, they should also close any P, which will
            // be taken care by followup check.
            curNodeId = popUpTo(NULL, tag_id, HTML_SCOPE_OPENING_LI);
        } else if (tag_id == el_dt || tag_id == el_dd) {
            curNodeId = popUpTo(NULL, 0, HTML_SCOPE_OPENING_DT_DD);
        } else if (tag_id == el_select) {
            curNodeId = popUpTo(NULL, tag_id, HTML_SCOPE_SELECT);
        }
        if (_lastP && tag_id >= EL_SPECIAL_CLOSING_P_START && tag_id <= EL_SPECIAL_CLOSING_P_END) {
            // All these should close a P "in button scope", meaning until a parent
            // with these tag names is met:
            //   html, table, td, th, caption, applet, marquee, object, template
            // These should all have closed any previous P when opened, except
            // applet, marquee, object, template - but to simplify things, we
            // made them close a P too. So, _lastP is always "in button scope".
            curNodeId = popUpTo(_lastP); // will set _lastP = NULL
            // Note: in "quirks mode", a TABLE should not close a P (should
            // we force this behaviour on old CHM files ? Having the table
            // close a P when it shouldn't will make the following text out
            // of P and possibly not styled as P).
        }
        if (tag_id >= el_h1 && tag_id <= el_h6) {
            // After possibly closing a P, H1...H6 close any H1...H6 direct ancestor
            curNodeId = popUpTo(NULL, 0, HTML_SCOPE_OPENING_H1_H6);
        } else if (curNodeId == el_option && (tag_id == el_optgroup || tag_id == el_option)) {
            // Close previous option
            curNodeId = popUpTo(_currNode);
        } else if (tag_id >= el_rbc && tag_id <= el_rp) { // ruby sub-elements
            // The HTML5 specs says that:
            // - we should do that only if there is a RUBY in scope (but we don't check that)
            // - RB and RTC should close implied end tags, meaning: RB RP RT RTC
            // - RP and RT should close implied end tags except RTC, meaning: RB RP RT
            // But they don't mention the old <RBC> that we want to support
            // If we do, we end up with these rules (x for HTML specs, o for our added RBC support)
            //               tags to close
            //     tag_id   RBC RB RTC RT RP
            //     RBC       o  o   o  o  o
            //     RB           x   x  x  x
            //     RTC       o  x   x  x  x
            //     RT        o  x      x  x
            //     RP        o  x      x  x
            if (tag_id == el_rbc || tag_id == el_rtc) {
                while (curNodeId >= el_rbc && curNodeId <= el_rp) {
                    curNodeId = popUpTo(_currNode);
                }
            } else if (tag_id == el_rb) {
                while (curNodeId >= el_rb && curNodeId <= el_rp) {
                    curNodeId = popUpTo(_currNode);
                }
            } else { // el_rt || el_rp
                while (curNodeId >= el_rbc && curNodeId <= el_rp && curNodeId != el_rtc) {
                    curNodeId = popUpTo(_currNode);
                }
            }
        }

        // Self closing will be handled in OnTagBody
        _curNodeIsSelfClosing = is_self_closing_tag;
    } else if (step == PARSER_STEP_TAG_CLOSING || step == PARSER_STEP_TAG_SELF_CLOSING) { // Closing, </tag_id> or <tag_id/>
        // We are responsible for poping up to and closing the provided tag_id,
        // or ignoring it if stopped or not found.
        if (is_self_closing_tag) {
            // We can ignore this closing tag, except in one case:
            // a standalone closing </BR> (so, not self closing)
            // Specs say we should insert a new/another one
            if (tag_id == el_br && step == PARSER_STEP_TAG_CLOSING) {
                OnTagOpen(U"", U"br");
                OnTagBody();
                OnTagClose(U"", U"br", true);
                return true;
            }
            return false; // ignored
        }
        if (tag_id == curNodeId) {
            // If closing current node, no need for more checks
            popUpTo(_currNode);
            return true;
        }
        if (tag_id == el_p && !_lastP) {
            // </P> without any previous <P> should emit <P></P>
            // Insert new one and pop it
            OnTagOpen(U"", U"p");
            OnTagBody();
            popUpTo(_currNode);
            return true;
        }
        if (tag_id > EL_SPECIAL_END) {
            // Inline elements don't close across specials
            curNodeId = popUpTo(NULL, tag_id, HTML_SCOPE_SPECIALS);
        } else if (tag_id >= el_h1 && tag_id <= el_h6) {
            // A closing Hn closes any other Hp
            curNodeId = popUpTo(NULL, tag_id, HTML_SCOPE_CLOSING_H1_H6);
        } else if (tag_id == el_li) {
            // </li> shouldn't close across OL/UL
            curNodeId = popUpTo(NULL, tag_id, HTML_SCOPE_LIST_ITEM);
            // Note: dt/dd (which have the same kind of auto-close previous
            // as LI for the opening tag) do not have any restriction, and
            // will use HTML_SCOPE_MAIN below
        } else if (tag_id >= el_table && tag_id <= el_td) {
            // Table sub-element: don't cross TABLE
            curNodeId = popUpTo(NULL, tag_id, HTML_SCOPE_TABLE);
        } else if (tag_id >= EL_SPECIAL_START) {
            // All other "specials" close across nearly everything
            // except TABLE/TH/TD/CAPTION
            curNodeId = popUpTo(NULL, tag_id, HTML_SCOPE_MAIN);
        } else {
            // Boxing elements are normally added by crengine after
            // "delete ldomElementWriter" (which calls onBodyExit()
            // which calls initNodeRendMethod()), so after we have
            // closed and pass by the element.
            // So, we shouldn't meet any.
            // But logically, they shouldn't have any limitation
            curNodeId = popUpTo(NULL, tag_id, HTML_SCOPE_NONE);
        }
        // SELECT should close any previous SELECT in HTML_SCOPE_SELECT,
        // which should contain only OPTGROUP and OPTION, but we don't
        // ensure that. So, we don't ensure this closing restriction.
    }

    // (Silences clang warning about 'curNodeId' is never read, if we
    // happen to not had the need to re-check it - but better to keep
    // updating it if we later add stuff that does use it)
    (void)curNodeId;

    return true;
}

bool ldomDocumentWriterFilter::CheckAndEnsureFosterParenting(lUInt16 tag_id) {
    if (!_currNode)
        return false;
    lUInt16 curNodeId = _currNode->getElement()->getNodeId();
    if (curNodeId >= el_table && curNodeId <= el_tr && curNodeId != el_caption) {
        if (tag_id < el_table || tag_id > el_td) {
            // Non table sub-element met as we expect only a table sub-element.
            // Ensure foster parenting: this node (and its content) is to be
            // inserted as a previous sibling of the table element we are in
            _curNodeBeforeFostering = NULL;
            // Look for the containing table element
            ldomElementWriter* elem = _currNode;
            while (elem) {
                if (elem->getElement()->getNodeId() == el_table) {
                    break;
                }
                elem = elem->_parent;
            }
            if (elem) { // found it
                _curNodeBeforeFostering = _currNode;
                _currNode = elem->_parent; // parent of table
                return true;               // Insert the new element in _currNode (the parent of this
                                           // table), before its last child (which is this table)
            }
        }
        // We're in a table, and we see an expected sub-table element: all is fine
        return false;
    } else if (_curFosteredNode) {
        // We've been foster parenting: if we see a table sub-element,
        // stop foster parenting and restore the original noce
        if (tag_id >= el_table && tag_id <= el_td) {
            popUpTo(_curFosteredNode);
            // popUpTo() has restored _currNode to _curNodeBeforeFostering and
            // reset _curFosteredNode and _curNodeBeforeFostering to NULL
        }
    }
    return false;
}

void ldomDocumentWriterFilter::ElementCloseHandler(ldomNode* node) {
    node->persist();
}

ldomNode* ldomDocumentWriterFilter::OnTagOpen(const lChar32* nsname, const lChar32* tagname) {
    // We expect from the parser to always have OnTagBody called
    // after OnTagOpen before any other OnTagOpen
    if (!_tagBodyCalled) {
        CRLog::error("OnTagOpen w/o parent's OnTagBody : %s", LCSTR(lString32(tagname)));
        crFatalError();
    }
    // _tagBodyCalled = false;
    // We delay setting _tagBodyCalled=false to below as we may create
    // additional wrappers before inserting this new element

    lUInt16 id = _document->getElementNameIndex(tagname);
    lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;

    // http://lib.ru/ books detection (a bit ugly to have this hacked
    // into ldomDocumentWriterFilter, but well, it's been there for ages
    // and it seems quite popular and expected to have crengine handle
    // Lib.ru books without any conversion needed).
    // Detection has been reworked to be done here (in OnTagOpen). It
    // was previously done in ElementCloseHandler/OnTagClose when closing
    // the elements, and as it removed the FORM node from the DOM, it
    // caused a display hash mismatch which made the cache invalid.
    // So, do it here and don't remove any node but make then hidden.
    // Lib.ru books (in the 2 formats that are supported, "Lib.ru html"
    // and "Fine HTML"), have this early in the document:
    //   <div align=right><form action=/INPROZ/ASTURIAS/asturias1_1.txt><select name=format><OPTION...>
    // Having a FORM child of a DIV with align=right is assumed to be
    // quite rare, so check for that.
    bool setDisplayNone = false;
    bool setParseAsPre = false;
    if (_libRuDocumentToDetect && id == el_form) {
        // At this point _currNode is still the parent of the FORM that is opening
        if (_currNode && _currNode->_element->getNodeId() == el_div) {
            ldomNode* node = _currNode->_element;
            lString32 style = node->getAttributeValue(attr_style);
            // align=right would have been translated to style="text-align: right"
            if (!style.empty() && style.pos("text-align: right", 0) >= 0) {
                _libRuDocumentDetected = true;
                // We can't set this DIV to be display:none as the element
                // has already had setNodeStyle() called and applied, so
                // it would take effect only on re-renderings (and would
                // cause a display hash mismatch).
                // So, we'll set it on the FORM just after it's created below
                setDisplayNone = true;
            }
        }
        // If the first FORM met doesn't match, no need keep detecting
        _libRuDocumentToDetect = false;
    }
    // Fixed 20180503: this was done previously in any case, but now only
    // if _libRuDocumentDetected. We still allow the old behaviour if
    // requested to keep previously recorded XPATHs valid.
    if (_libRuDocumentDetected || _document->getDOMVersionRequested() < 20180503) {
        // Patch for bad LIB.RU books - BR delimited paragraphs
        // in "Fine HTML" format, that appears as:
        //   <br>&nbsp; &nbsp; &nbsp; Viento fuerte, 1950
        //   <br>&nbsp; &nbsp; &nbsp; Spellcheck [..., with \n every 76 chars]
        if (id == el_br || id == el_dd) {
            // Replace such BR with P
            id = el_p;
            _libRuParagraphStart = true; // to trim leading &nbsp;
        } else {
            _libRuParagraphStart = false;
        }
        if (_libRuDocumentDetected && id == el_pre) {
            // "Lib.ru html" format is actually minimal HTML with
            // the text wrapped in <PRE>. We will parse this text
            // to build proper HTML with each paragraph wrapped
            // in a <P> (this is done by the XMLParser when we give
            // it TXTFLG_PRE_PARA_SPLITTING).
            // Once that is detected, we don't want it to be PRE
            // anymore (so that on re-renderings, it's not handled
            // as white-space: pre), so we're swapping this PRE with
            // a DIV element. But we need to still parse the text
            // when building the DOM as PRE.
            id = el_div;
            ldomNode* n = _currNode ? _currNode->getElement() : NULL;
            if (n && n->getNodeId() == el_pre) {
                // Also close any previous PRE that would have been
                // auto-closed if we kept PRE as PRE (from now on,
                // we'll convert PRE to DIV), as this unclosed PRE
                // would apply to all the text.
                _currNode = pop(_currNode, el_pre);
            } else if (n && n->getNodeId() == el_div && n->hasAttribute(attr_ParserHint) &&
                       n->getAttributeValue(attr_ParserHint) == U"ParseAsPre") {
                // Also close any previous PRE we already masqueraded
                // as <DIV ParserHint="ParseAsPre">
                _currNode = pop(_currNode, el_div);
            }
            // Below, we'll then be inserting a DIV, which won't be TXTFLG_PRE.
            // We'll need to re-set _flags to be TXTFLG_PRE in our OnTagBody(),
            // after it has called the superclass's OnTagBody(),
            // as ldomDocumentWriter::OnTagBody() will call onBodyEnter() which
            // will have set default styles (so, not TXTFLG_PRE for DIV as its
            // normal style is "white-space: normal").
            // We'll add the attribute ParserHint="ParseAsPre" below so
            // we know it was a PRE and do various tweaks.
            setParseAsPre = true;
        }
    }

#if MATHML_SUPPORT == 1
    if ((_currNode && _currNode->_insideMathML) || (id == el_math)) {
        // This may create a wrapping mathBox around this new element
        _mathMLHelper.handleMathMLtag(this, MATHML_STEP_BEFORE_NEW_CHILD, id);
    }
#endif

    bool tag_accepted = true;
    bool insert_before_last_child = false;
    if (_document->getDOMVersionRequested() >= 20200824) { // A little bit more HTML5 conformance
        if (id == el_image)
            id = el_img;
        if (tagname && tagname[0] == '?') {
            // The XML parser feeds us XML processing instructions like '<?xml ... ?>'
            // Firefox wraps them in a comment <!--?xml ... ?-->.
            // As we ignore comments, ignore them too.
            tag_accepted = false;
        } else if (CheckAndEnsureFosterParenting(id)) {
            // https://html.spec.whatwg.org/multipage/parsing.html#foster-parent
            // If non-sub-table element opening while we're still
            // inside sub-table non-TD/TH elements, we should
            // do foster parenting: insert the node as the previous
            // sibling of the TABLE element we're dealing with
            insert_before_last_child = true;
            // As we'll be inserting a node before the TABLE, which
            // already had its style applied, some CSS selectors matches
            // might no more be valid (i.e. :first-child, DIV + TABLE),
            // so styles could change on the next re-rendering.
            // We don't check if we actually had such selectors as that
            // is complicated from here: we just set styles to be invalid
            // so they are re-computed once the DOM is fully built.
            _document->setNodeStylesInvalidIfLoading();
        } else {
            tag_accepted = AutoOpenClosePop(PARSER_STEP_TAG_OPENING, id);
        }
    } else {
        AutoClose(id, true);
    }

    // From now on, we don't create/close any elements, so expect
    // the next event to be OnTagBody (except OnTagAttribute)
    _tagBodyCalled = false;

    if (!tag_accepted) {
        // Don't create the element
        // If not accepted, the HTML parser will still call OnTagBody, and might
        // call OnTagAttribute before that. We should ignore them until OnTagBody.
        // No issue with OnTagClose, that can usually ignore stuff.
        _curTagIsIgnored = true;
        return _currNode ? _currNode->getElement() : NULL;
    }

    // Set a flag for OnText to accumulate the content of any <HEAD><STYLE>
    // (We do that after the autoclose above, so that with <HEAD><META><STYLE>,
    // the META is properly closed and we find HEAD as the current node.)
    if (id == el_style && _currNode && _currNode->getElement()->getNodeId() == el_head) {
        _inHeadStyle = true;
    }

    _currNode = new ldomElementWriter(_document, nsid, id, _currNode, insert_before_last_child);
    _flags = _currNode->getFlags();

    if (insert_before_last_child) {
        _curFosteredNode = _currNode;
    }

    if (_document->getDOMVersionRequested() >= 20200824 && id == el_p) {
        // To avoid checking DOM ancestors with the numerous tags that close a P
        _lastP = _currNode;
    }

    // Some libRu tweaks:
    if (setParseAsPre) {
        // Set an attribute on the DIV we just added
        _currNode->getElement()->setAttributeValue(LXML_NS_NONE, attr_ParserHint, U"ParseAsPre");
        // And set this global flag as we'll need to re-enable PRE (as it
        // will be reset by ldomDocumentWriter::OnTagBody() as we won't have
        // proper CSS white-space:pre inheritance) and XMLParser flags.
        _libRuParseAsPre = true;
    }
    if (setDisplayNone) {
        // Hide the FORM that was used to detect libRu,
        // now that currNode is the FORM element
        appendStyle(U"display: none");
    }

    //logfile << " !o!\n";
    return _currNode->getElement();
}

/// called after > of opening tag (when entering tag body)
// Note to avoid confusion: all tags HAVE a body (their content), so this
// is called on all tags.
void ldomDocumentWriterFilter::OnTagBody() {
    _tagBodyCalled = true;
    if (_curTagIsIgnored) {
        _curTagIsIgnored = false; // Done with this ignored tag
        // We don't want ldomDocumentWriter::OnTagBody() to re-init
        // the current node styles (as we ignored this element,
        // _currNode is the previous node, already initNodeStyle()'d)
        return;
    }

    // This superclass OnTagBody() will initNodeStyle() on this node.
    // Some specific handling for the <BODY> tag to deal with HEAD STYLE
    // and LINK is also done there.
    ldomDocumentWriter::OnTagBody();

    if (_curNodeIsSelfClosing) {
        // Now that styles are set, we can close the element
        // Let's have it closed properly with flags correctly re set, and so
        // that specific handling in OnTagClose() is done (ex. for <LINK>)
        OnTagClose(NULL, NULL, true);
        return;
    }

    if (_libRuDocumentDetected) {
        if (_libRuParseAsPre) {
            // The OnTagBody() above might have cancelled TXTFLG_PRE
            // (that the ldomElementWriter inherited from its parent)
            // when ensuring proper CSS white-space inheritance.
            // Re-enable it
            _currNode->_flags |= TXTFLG_PRE;
            // Also set specific XMLParser flags so it spits out
            // <P>... for each paragraph of plain text, so that
            // we get some nice HTML instead
            _flags = TXTFLG_PRE | TXTFLG_PRE_PARA_SPLITTING | TXTFLG_TRIM;
        }
    }
}

void ldomDocumentWriterFilter::OnAttribute(const lChar32* nsname, const lChar32* attrname, const lChar32* attrvalue) {
    //logfile << "ldomDocumentWriter::OnAttribute() [" << nsname << ":" << attrname << "]";
    //if ( nsname && nsname[0] )
    //    lStr_lowercase( const_cast<lChar32 *>(nsname), lStr_len(nsname) );
    //lStr_lowercase( const_cast<lChar32 *>(attrname), lStr_len(attrname) );

    //CRLog::trace("OnAttribute(%s, %s)", LCSTR(lString32(attrname)), LCSTR(lString32(attrvalue)));

    if (_curTagIsIgnored) { // Ignore attributes if tag was ignored
        return;
    }

    // ldomDocumentWriterFilter is used for HTML/CHM/PDB (not with EPUBs).
    // We translate some attributes (now possibly deprecated) to their
    // CSS style equivalent, globally or for some elements only.
    // https://developer.mozilla.org/en-US/docs/Web/HTML/Attributes
    lUInt16 id = _currNode->_element->getNodeId();

// Not sure this is to be done here: we get attributes as they are read,
// so possibly before or after a style=, that the attribute may override.
// Hopefully, a document use either one or the other.
// (Alternative: in lvrend.cpp when used, as fallback when there is
// none specified in node->getStyle().)
#if MATHML_SUPPORT == 1
    // We should really remove this handling below and support them via CSS
    // for HTML5 elements that support them.
    // In the meantime, just don't mess with MathML
    if (id >= EL_MATHML_START && id <= EL_MATHML_END) {
        lUInt16 attr_ns = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
        lUInt16 attr_id = (attrname && attrname[0]) ? _document->getAttrNameIndex(attrname) : 0;
        _currNode->addAttribute(attr_ns, attr_id, attrvalue);
        return;
    }
#endif

    // HTML align= => CSS text-align:
    // Done for all elements, except IMG and TABLE (for those, it should
    // translate to float:left/right, which is ensured by epub.css)
    // Should this be restricted to some specific elements?
    if (!lStr_cmp(attrname, "align") && (id != el_img) && (id != el_table)) {
        lString32 align = lString32(attrvalue).lowercase();
        if (align == U"justify")
            appendStyle(U"text-align: justify");
        else if (align == U"left")
            appendStyle(U"text-align: left");
        else if (align == U"right")
            appendStyle(U"text-align: right");
        else if (align == U"center")
            appendStyle(U"text-align: center");
        return;
    }

    // For the table & friends elements where we do support the following styles,
    // we translate these deprecated attributes to their style equivalents:
    //
    // HTML valign= => CSS vertical-align: only for TH & TD (as lvrend.cpp
    // only uses it with table cells (erm_final or erm_block))
    if (id == el_th || id == el_td) {
        // Default rendering for cells is valign=baseline
        if (!lStr_cmp(attrname, "valign")) {
            lString32 valign = lString32(attrvalue).lowercase();
            if (valign == U"top")
                appendStyle(U"vertical-align: top");
            else if (valign == U"middle")
                appendStyle(U"vertical-align: middle");
            else if (valign == U"bottom")
                appendStyle(U"vertical-align: bottom");
            return;
        }
    }
    // HTML width= => CSS width: only for TH, TD and COL (as lvrend.cpp
    // only uses it with erm_table_column and table cells)
    // Note: with IMG, lvtextfm LFormattedText::AddSourceObject() only uses
    // style, and not attributes: <img width=100 height=50> would not be used.
    if (id == el_th || id == el_td || id == el_col) {
        if (!lStr_cmp(attrname, "width")) {
            lString32 val = lString32(attrvalue);
            const lChar32* s = val.c_str();
            bool is_pct = false;
            int n = 0;
            if (s && s[0]) {
                for (int i = 0; s[i]; i++) {
                    if (s[i] >= '0' && s[i] <= '9') {
                        n = n * 10 + (s[i] - '0');
                    } else if (s[i] == '%') {
                        is_pct = true;
                        break;
                    }
                }
                if (n > 0) {
                    val = lString32("width: ");
                    val.appendDecimal(n);
                    val += is_pct ? "%" : "px"; // CSS pixels
                    appendStyle(val.c_str());
                }
            }
            return;
        }
    }

    // Othewise, add the attribute
    lUInt16 attr_ns = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
    lUInt16 attr_id = (attrname && attrname[0]) ? _document->getAttrNameIndex(attrname) : 0;

    _currNode->addAttribute(attr_ns, attr_id, attrvalue);

    //logfile << " !a!\n";
}

/// called on closing tag
void ldomDocumentWriterFilter::OnTagClose(const lChar32* /*nsname*/, const lChar32* tagname, bool self_closing_tag) {
    if (!_tagBodyCalled) {
        CRLog::error("OnTagClose w/o parent's OnTagBody : %s", LCSTR(lString32(tagname)));
        crFatalError();
    }
    if (!_currNode || !_currNode->getElement()) {
        _errFlag = true;
        return;
    }

    //lUInt16 nsid = (nsname && nsname[0]) ? _document->getNsNameIndex(nsname) : 0;
    lUInt16 curNodeId = _currNode->getElement()->getNodeId();
    lUInt16 id = tagname ? _document->getElementNameIndex(tagname) : curNodeId;
    _errFlag |= (id != curNodeId); // (we seem to not do anything with _errFlag)
    // We should expect the tagname we got to be the same as curNode's element name,
    // but it looks like we may get an upper closing tag, that pop() or AutoClose()
    // below might handle. So, here below, we check that both id and curNodeId match
    // the element id we check for.

    if (_libRuDocumentToDetect && id == el_div) {
        // No need to try detecting after we see a closing </DIV>,
        // as the FORM we look for is in the first DIV
        _libRuDocumentToDetect = false;
    }
    if (_libRuDocumentDetected && id == el_pre) {
        // Also, if we're about to close the original PRE that we masqueraded
        // as DIV and that has enabled _libRuParseAsPre, reset it.
        // (In Lib.ru books, it seems a PRE is never closed, or only at
        // the end by another PRE where it doesn't matter if we keep that flag.)
        ldomNode* n = _currNode->getElement();
        if (n->getNodeId() == el_div && n->hasAttribute(attr_ParserHint) &&
            n->getAttributeValue(attr_ParserHint) == U"ParseAsPre") {
            _libRuParseAsPre = false;
        }
    }

    // Parse <link rel="stylesheet">, put the css file link in _stylesheetLinks,
    // they will be added to <body><stylesheet> when we meet <BODY>
    // (duplicated in ldomDocumentWriter::OnTagClose)
    if (id == el_link && curNodeId == el_link) { // link node
        ldomNode* n = _currNode->getElement();
        if (n->getParentNode() && n->getParentNode()->getNodeId() == el_head &&
            lString32(n->getAttributeValue("rel")).lowercase() == U"stylesheet" &&
            lString32(n->getAttributeValue("type")).lowercase() == U"text/css") {
            lString32 href = n->getAttributeValue("href");
            lString32 stylesheetFile = LVCombinePaths(_document->getCodeBase(), href);
            CRLog::debug("Internal stylesheet file: %s", LCSTR(stylesheetFile));
            // We no more apply it immediately: it will be when <BODY> is met
            // _document->setDocStylesheetFileName(stylesheetFile);
            // _document->applyDocumentStyleSheet();
            _stylesheetLinks.add(stylesheetFile);
        }
    }

    // HTML title detection
    if (id == el_title && curNodeId == el_title && _currNode->_element->getParentNode() &&
        _currNode->_element->getParentNode()->getNodeId() == el_head) {
        lString32 s = _currNode->_element->getText();
        s.trim();
        if (!s.empty()) {
            // TODO: split authors, title & series
            _document->getProps()->setString(DOC_PROP_TITLE, s);
        }
    }

#if MATHML_SUPPORT == 1
    if (_currNode->_insideMathML) {
        if (_mathMLHelper.handleMathMLtag(this, MATHML_STEP_NODE_CLOSING, id)) {
            // curnode may have changed
            curNodeId = _currNode->getElement()->getNodeId();
            id = tagname ? _document->getElementNameIndex(tagname) : curNodeId;
            _errFlag |= (id != curNodeId); // (we seem to not do anything with _errFlag)
        }
    }
#endif

    if (_document->getDOMVersionRequested() >= 20200824) { // A little bit more HTML5 conformance
        if (_curNodeIsSelfClosing) {                       // Internal call (not from XMLParser)
            _currNode = pop(_currNode, id);
            _curNodeIsSelfClosing = false;
        } else {
            if (id == el_image)
                id = el_img;
            AutoOpenClosePop(self_closing_tag ? PARSER_STEP_TAG_SELF_CLOSING : PARSER_STEP_TAG_CLOSING, id);
        }
    } else {
        //======== START FILTER CODE ============
        AutoClose(curNodeId, false);
        //======== END FILTER CODE ==============
        // save closed element
        // ldomNode * closedElement = _currNode->getElement();
        _currNode = pop(_currNode, id);
        // _currNode is now the parent
    }

#if MATHML_SUPPORT == 1
    if (_currNode->_insideMathML) {
        // This may close wrapping mathBoxes
        _mathMLHelper.handleMathMLtag(this, MATHML_STEP_NODE_CLOSED, id);
    }
#endif

    if (_currNode) {
        _flags = _currNode->getFlags();
        if (_libRuParseAsPre) {
            // Re-set specific parser flags
            _flags |= TXTFLG_PRE | TXTFLG_PRE_PARA_SPLITTING | TXTFLG_TRIM;
        }
    }

    if (id == _stopTagId) {
        //CRLog::trace("stop tag found, stopping...");
        _parser->Stop();
    }
    //logfile << " !c!\n";
}

/// called on text
void ldomDocumentWriterFilter::OnText(const lChar32* text, int len, lUInt32 flags) {
    // Accumulate <HEAD><STYLE> content
    if (_inHeadStyle) {
        _headStyleText << lString32(text, len);
        _inHeadStyle = false;
        return;
    }

    if (_document->getDOMVersionRequested() >= 20200824) { // A little bit more HTML5 conformance
        // We can get text before any node (it should then have <html><body> emited before it),
        // but we might get spaces between " <html> <head> <title>The title <br>The content".
        // Try to handle that correctly.
        if (!_bodyTagSeen) {
            // While not yet in BODY, when in HTML or HEAD, ignore empty
            // text (as non empty text will create BODY)
            if (!_currNode || _currNode->getElement()->isRoot() ||
                _currNode->getElement()->getNodeId() == el_html ||
                _currNode->getElement()->getNodeId() == el_head) {
                if (!IsEmptySpace(text, len)) {
                    // Non-empty text: have implicit HTML or BODY tags created and HEAD closed
                    AutoOpenClosePop(PARSER_STEP_TEXT, 0);
                }
            }
        }
    }
    //logfile << "lxmlDocumentWriter::OnText() fpos=" << fpos;
    if (_currNode) {
        lUInt16 curNodeId = _currNode->getElement()->getNodeId();
        if (_document->getDOMVersionRequested() < 20200824) {
            AutoClose(curNodeId, false);
        }
        if ((_flags & XML_FLAG_NO_SPACE_TEXT) && IsEmptySpace(text, len) && !(flags & TXTFLG_PRE))
            return;
        bool insert_before_last_child = false;
        if (_document->getDOMVersionRequested() >= 20200824) {
            // If we're inserting text while in table sub-elements that
            // don't accept text, have it foster parented
            if (curNodeId >= el_table && curNodeId <= el_tr && curNodeId != el_caption) {
                if (!IsEmptySpace(text, len)) {
                    if (CheckAndEnsureFosterParenting(el_NULL)) {
                        insert_before_last_child = true;
                    }
                }
            }
        } else {
            // Previously, text in table sub-elements (only table elements and
            // self-closing elements have _allowText=false) had any text in between
            // table elements dropped (but not elements! with "<table>abc<div>def",
            // "abc" was dropped, but not "def")
            if (!_currNode->_allowText)
                return;
        }
#if MATHML_SUPPORT == 1
        if (_currNode->_insideMathML) {
            lString32 math_text = _mathMLHelper.getMathMLAdjustedText(_currNode->getElement(), text, len);
            if (!math_text.empty()) {
                _mathMLHelper.handleMathMLtag(this, MATHML_STEP_BEFORE_NEW_CHILD, el_NULL);
                _currNode->onText(math_text.c_str(), math_text.length(), flags, insert_before_last_child);
            }
        } else // skip the next checks if we are _insideMathML
#endif
                if (!_libRuDocumentDetected) {
            _currNode->onText(text, len, flags, insert_before_last_child);
        } else { // Lib.ru text cleanup
            if (_libRuParagraphStart) {
                // Cleanup "Fine HTML": "<br>&nbsp; &nbsp; &nbsp; Viento fuerte, 1950"
                while (*text == 160 && len > 0) {
                    text++;
                    len--;
                    while (*text == ' ' && len > 0) {
                        text++;
                        len--;
                    }
                }
                _libRuParagraphStart = false;
            }
            // Handle "Lib.ru html" paragraph, parsed from the nearly plaintext
            // by XMLParser with TXTFLG_PRE | TXTFLG_PRE_PARA_SPLITTING | TXTFLG_TRIM
            bool autoPara = flags & TXTFLG_PRE;
            int leftSpace = 0;
            const lChar32* paraTag = NULL;
            bool isHr = false;
            if (autoPara) {
                while ((*text == ' ' || *text == '\t' || *text == 160) && len > 0) {
                    text++;
                    len--;
                    leftSpace += (*text == '\t') ? 8 : 1;
                }
                paraTag = leftSpace > 8 ? U"h2" : U"p";
                lChar32 ch = 0;
                bool sameCh = true;
                for (int i = 0; i < len; i++) {
                    if (!ch)
                        ch = text[i];
                    // We would need this to have HR work:
                    //   else if ( i == len-1 && text[i] == ' ' ) {
                    //      // Ignore a trailing space we may get
                    //      // Note that some HR might be missed when the
                    //      // "----" directly follows some indented text.
                    //   }
                    // but by fixing it, we'd remove a P and have XPointers
                    // like /html/body/div/p[14]/text().113 reference the wrong P,
                    // so keep doing bad to not mess past highlights...
                    else if (ch != text[i]) {
                        sameCh = false;
                        break;
                    }
                }
                if (!ch)
                    sameCh = false;
                if ((ch == '-' || ch == '=' || ch == '_' || ch == '*' || ch == '#') && sameCh)
                    isHr = true;
            }
            if (isHr) {
                OnTagOpen(NULL, U"hr");
                OnTagBody();
                OnTagClose(NULL, U"hr");
            } else if (len > 0) {
                if (autoPara) {
                    OnTagOpen(NULL, paraTag);
                    OnTagBody();
                }
                _currNode->onText(text, len, flags, insert_before_last_child);
                if (autoPara)
                    OnTagClose(NULL, paraTag);
            }
        }
        if (insert_before_last_child) {
            // We have no _curFosteredNode to pop, so just restore
            // the previous table node
            _currNode = _curNodeBeforeFostering;
            _curNodeBeforeFostering = NULL;
            _curFosteredNode = NULL;
        }
    }
    //logfile << " !t!\n";
}

ldomDocumentWriterFilter::ldomDocumentWriterFilter(ldomDocument* document, bool headerOnly, const char*** rules)
        : ldomDocumentWriter(document, headerOnly)
        , _libRuDocumentToDetect(true)
        , _libRuDocumentDetected(false)
        , _libRuParagraphStart(false)
        , _libRuParseAsPre(false)
        , _styleAttrId(0)
        , _classAttrId(0)
        , _tagBodyCalled(true)
        , _htmlTagSeen(false)
        , _headTagSeen(false)
        , _bodyTagSeen(false)
        , _curNodeIsSelfClosing(false)
        , _curTagIsIgnored(false)
        , _curNodeBeforeFostering(NULL)
        , _curFosteredNode(NULL)
        , _lastP(NULL) {
    if (_document->getDOMVersionRequested() >= 20200824) {
        // We're not using the provided rules, but hardcoded ones in AutoOpenClosePop()
        return;
    }
    lUInt16 i;
    for (i = 0; i < MAX_ELEMENT_TYPE_ID; i++)
        _rules[i] = NULL;
    lUInt16 items[MAX_ELEMENT_TYPE_ID];
    for (i = 0; rules[i]; i++) {
        const char** rule = rules[i];
        lUInt16 j;
        for (j = 0; rule[j] && j < MAX_ELEMENT_TYPE_ID; j++) {
            const char* s = rule[j];
            items[j] = _document->getElementNameIndex(lString32(s).c_str());
        }
        if (j >= 1) {
            lUInt16 id = items[0];
            _rules[id] = new lUInt16[j];
            for (int k = 0; k < j; k++) {
                _rules[id][k] = k == j - 1 ? 0 : items[k + 1];
            }
        }
    }
}

ldomDocumentWriterFilter::~ldomDocumentWriterFilter() {
    if (_document->getDOMVersionRequested() >= 20200824) {
        return;
    }
    for (int i = 0; i < MAX_ELEMENT_TYPE_ID; i++) {
        if (_rules[i])
            delete[] _rules[i];
    }
}
