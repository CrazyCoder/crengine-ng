/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007-2013 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2015,2016 Yifei(Frank) ZHU <fredyifei@gmail.com>        *
 *   Copyright (C) 2016 Qingping Hou <dave2008713@gmail.com>               *
 *   Copyright (C) 2020 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2017-2021 poire-z <poire-z@users.noreply.github.com>    *
 *   Copyright (C) 2020,2023 Aleksey Chernov <valexlin@gmail.com>          *
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

#include <ldomxpointer.h>
#include <ldomxpointerex.h>
#include <lvrend.h>
#include <lvstreamutils.h>
#include <ldomdocument.h>
#include <fb2def.h>
#include <crlog.h>

#include "renderrectaccessor.h"
#include "ldomnodeidpredicate.h"
#include "writenodeex.h"

static bool isBoxingNode(ldomNode* node) {
    // In the context this is used (xpointers), handle pseudoElems (that don't
    // box anything) just as boxing nodes: ignoring them in XPointers.
    return node->isBoxingNode(true);
}

static bool isTextNode(ldomNode* node) {
    return (node && node->isText());
}

static bool notNull(ldomNode* node) {
    return (NULL != node);
}

/// return parent final node, if found
ldomNode* ldomXPointer::getFinalNode() const {
    ldomNode* node = getNode();
    for (;;) {
        if (!node)
            return NULL;
        if (node->getRendMethod() == erm_final)
            return node;
        node = node->getParentNode();
    }
}

/// return true is this node is a final node
bool ldomXPointer::isFinalNode() const {
    ldomNode* node = getNode();
    if (!node)
        return false;
    if (node->getRendMethod() == erm_final)
        return true;
    return false;
}

/// get pointer for relative path
ldomXPointer ldomXPointer::relative(lString32 relativePath) {
    return getDocument()->createXPointer(getNode(), relativePath);
}

/// returns coordinates of pointer inside formatted document
lvPoint ldomXPointer::toPoint(bool extended) const {
    lvRect rc;
    if (!getRect(rc, extended))
        return lvPoint(-1, -1);
    return rc.topLeft();
}

/// returns caret rectangle for pointer inside formatted document
// (with extended=true, consider paddings and borders)
// Note that extended / ldomXPointer::getRectEx() is only used (by cre.cpp)
// when dealing with hyphenated words, getting each char width, char by char.
// So we return the char width (and no more the word width) of the char
// pointed to by this XPointer (unlike ldomXRange::getRectEx() which deals
// with a range between 2 XPointers).
bool ldomXPointer::getRect(lvRect& rect, bool extended, bool adjusted) const {
    //CRLog::trace("ldomXPointer::getRect()");
    if (isNull())
        return false;
    ldomNode* p = isElement() ? getNode() : getNode()->getParentNode();
    ldomNode* p0 = p;
    ldomNode* finalNode = NULL;
    if (!p) {
        //CRLog::trace("ldomXPointer::getRect() - p==NULL");
        return false;
    }
    ldomDocument* doc = p->getDocument();
    //printf("getRect( p=%08X type=%d )\n", (unsigned)p, (int)p->getNodeType() );
    if (!doc) {
        //CRLog::trace("ldomXPointer::getRect() - p->getDocument()==NULL");
        return false;
    }
    ldomNode* mainNode = doc->getRootNode();
    for (; p; p = p->getParentNode()) {
        int rm = p->getRendMethod();
        if (rm == erm_final) {
            if (doc->getDOMVersionRequested() < 20180524 && p->getStyle()->display == css_d_list_item_legacy) {
                // This legacy rendering of list item is now erm_final, but
                // can contain other real erm_final nodes.
                // So, if we found an erm_final, and if we find this erm_final
                // when going up, we should use it (unlike in next case).
                // (This is needed to correctly display highlights on books opened
                // with some older DOM_VERSION.)
                finalNode = p;
            } else {
                // With floats, we may get multiple erm_final when walking up
                // to root node: keep the first one met (but go on up to the
                // root node in case we're in some upper erm_invisible).
                if (!finalNode)
                    finalNode = p; // found final block
            }
        } else if (p->getRendMethod() == erm_invisible) {
            return false; // invisible !!!
        }
        if (p == mainNode)
            break;
    }

    if (finalNode == NULL) {
        lvRect rc;
        p0->getAbsRect(rc);
        //CRLog::debug("node w/o final parent: %d..%d", rc.top, rc.bottom);
    }

    if (finalNode != NULL) {
        lvRect rc;
        finalNode->getAbsRect(rc, extended); // inner=true if extended=true
        if (rc.height() == 0 && rc.width() > 0) {
            rect = rc;
            rect.bottom++;
            return true;
        }
        RenderRectAccessor fmt(finalNode);
        //if ( !fmt )
        //    return false;

        // When in enhanced rendering mode, we can get the FormattedText coordinates
        // and its width (inner_width) directly
        int inner_width;
        if (RENDER_RECT_HAS_FLAG(fmt, INNER_FIELDS_SET)) {
            inner_width = fmt.getInnerWidth();
            // if extended=true, we got directly the adjusted rc.top and rc.left
        } else {
            // In legacy mode, we just got the erm_final coordinates, and we must
            // compute and remove left/top border and padding (using rc.width() as
            // the base for % is wrong here)
            int padding_left = measureBorder(finalNode, 3) + lengthToPx(finalNode, finalNode->getStyle()->padding[0], rc.width());
            int padding_right = measureBorder(finalNode, 1) + lengthToPx(finalNode, finalNode->getStyle()->padding[1], rc.width());
            inner_width = fmt.getWidth() - padding_left - padding_right;
            if (extended) {
                int padding_top = measureBorder(finalNode, 0) + lengthToPx(finalNode, finalNode->getStyle()->padding[2], rc.width());
                rc.top += padding_top;
                rc.left += padding_left;
                // rc.right += padding_left; // wrong, but not used
                // rc.bottom += padding_top; // wrong, but not used
            }
        }

        // Get the formatted text, so we can look where in it is this XPointer
        LFormattedTextRef txtform;
        finalNode->renderFinalBlock(txtform, &fmt, inner_width);

        ldomNode* node = getNode();
        int offset = getOffset();
        ////        ldomXPointerEx xp(node, offset);
        ////        if ( !node->isText() ) {
        ////            //ldomXPointerEx xp(node, offset);
        ////            xp.nextVisibleText();
        ////            node = xp.getNode();
        ////            offset = xp.getOffset();
        ////        }
        //        if ( node->isElement() ) {
        //            if ( offset>=0 ) {
        //                //
        //                if ( offset>= (int)node->getChildCount() ) {
        //                    node = node->getLastTextChild();
        //                    if ( node )
        //                        offset = node->getText().length();
        //                    else
        //                        return false;
        //                } else {
        //                    for ( int ci=offset; ci<(int)node->getChildCount(); ci++ ) {
        //                        ldomNode * child = node->getChildNode( offset );
        //                        ldomNode * txt = txt = child->getFirstTextChild( true );
        //                        if ( txt ) {
        //                            node = txt;
        ////                            lString32 s = txt->getText();
        ////                            CRLog::debug("text: [%d] '%s'", s.length(), LCSTR(s));
        //                            break;
        //                        }
        //                    }
        //                    if ( !node->isText() )
        //                        return false;
        //                    offset = 0;
        //                }
        //            }
        //        }

        // text node
        int srcIndex = -1;
        int srcLen = -1;
        int lastIndex = -1;
        int lastLen = -1;
        int lastOffset = -1;
        ldomXPointerEx xp(node, offset);
        for (int i = 0; i < txtform->GetSrcCount(); i++) {
            const src_text_fragment_t* src = txtform->GetSrcInfo(i);
            if (src->flags & LTEXT_SRC_IS_FLOAT) // skip floats
                continue;
            bool isObject = (src->flags & LTEXT_SRC_IS_OBJECT) != 0;
            if (src->object == node) {
                srcIndex = i;
                srcLen = isObject ? 0 : src->u.t.len;
                break;
            }
            lastIndex = i;
            lastLen = isObject ? 0 : src->u.t.len;
            lastOffset = isObject ? 0 : src->u.t.offset;
            ldomXPointerEx xp2((ldomNode*)src->object, lastOffset);
            if (xp2.compare(xp) > 0) {
                srcIndex = i;
                srcLen = lastLen;
                offset = lastOffset;
                break;
            }
        }
        if (srcIndex == -1) {
            if (lastIndex < 0)
                return false;
            srcIndex = lastIndex;
            srcLen = lastLen;
            offset = lastOffset;
        }

        // Some state for non-linear bidi word search
        int nearestForwardSrcIndex = -1;
        int nearestForwardSrcOffset = -1;
        lvRect bestBidiRect = lvRect();
        bool hasBestBidiRect = false;

        for (int l = 0; l < txtform->GetLineCount(); l++) {
            const formatted_line_t* frmline = txtform->GetLineInfo(l);
            bool line_is_bidi = frmline->flags & LTEXT_LINE_IS_BIDI;
            for (int w = 0; w < (int)frmline->word_count; w++) {
                const formatted_word_t* word = &frmline->words[w];
                bool word_is_rtl = word->flags & LTEXT_WORD_DIRECTION_IS_RTL;
                bool lastWord = (l == txtform->GetLineCount() - 1 && w == frmline->word_count - 1);

                if (line_is_bidi) {
                    // When line is bidi, src text nodes may be shuffled, so we can't
                    // just be done when meeting a forward src in logical order.
                    // We'd better have a dedicated searching code to not mess with
                    // the visual=logical order generic code below.
                    // todo: see if additional tweaks according to
                    // frmline->flags&LTEXT_LINE_PARA_IS_RTL may help adjusting
                    // char rects depending on it vs word_is_rtl.
                    if (word->src_text_index >= srcIndex || lastWord) {
                        // Found word from same or forward src line
                        if (word->src_text_index > srcIndex &&
                            (nearestForwardSrcIndex == -1 ||
                             word->src_text_index < nearestForwardSrcIndex ||
                             (word->src_text_index == nearestForwardSrcIndex &&
                              word->u.t.start < nearestForwardSrcOffset))) {
                            // Found some word from a forward src that is nearest than previously found one:
                            // get its start as a possible best result.
                            bestBidiRect.top = rc.top + frmline->y;
                            bestBidiRect.bottom = bestBidiRect.top + frmline->height;
                            if (word_is_rtl) {
                                bestBidiRect.right = word->x + word->width + rc.left + frmline->x;
                                bestBidiRect.left = bestBidiRect.right - 1;
                            } else {
                                bestBidiRect.left = word->x + rc.left + frmline->x;
                                if (extended) {
                                    if (word->flags & (LTEXT_WORD_IS_OBJECT | LTEXT_WORD_IS_INLINE_BOX) && word->width > 0)
                                        bestBidiRect.right = bestBidiRect.left + word->width; // width of image
                                    else
                                        bestBidiRect.right = bestBidiRect.left + 1;
                                }
                            }
                            hasBestBidiRect = true;
                            nearestForwardSrcIndex = word->src_text_index;
                            if (word->flags & (LTEXT_WORD_IS_OBJECT | LTEXT_WORD_IS_INLINE_BOX))
                                nearestForwardSrcOffset = 0;
                            else
                                nearestForwardSrcOffset = word->u.t.start;
                        } else if (word->src_text_index == srcIndex) {
                            // Found word in that exact source text node
                            if (word->flags & (LTEXT_WORD_IS_OBJECT | LTEXT_WORD_IS_INLINE_BOX)) {
                                // An image is the single thing in its srcIndex
                                rect.top = rc.top + frmline->y;
                                rect.bottom = rect.top + frmline->height;
                                rect.left = word->x + rc.left + frmline->x;
                                if (word->width > 0)
                                    rect.right = rect.left + word->width; // width of image
                                else
                                    rect.right = rect.left + 1;
                                return true;
                            }
                            // Target is in this text node. We may not find it part
                            // of a word, so look at all words and keep the nearest
                            // (forward if possible) in case we don't find an exact one
                            if (word->u.t.start > offset) { // later word in logical order
                                if (nearestForwardSrcIndex != word->src_text_index ||
                                    word->u.t.start <= nearestForwardSrcOffset) {
                                    bestBidiRect.top = rc.top + frmline->y;
                                    bestBidiRect.bottom = bestBidiRect.top + frmline->height;
                                    if (word_is_rtl) { // right edge of next logical word, as it is drawn on the left
                                        bestBidiRect.right = word->x + word->width + rc.left + frmline->x;
                                        bestBidiRect.left = bestBidiRect.right - 1;
                                    } else { // left edge of next logical word, as it is drawn on the right
                                        bestBidiRect.left = word->x + rc.left + frmline->x;
                                        bestBidiRect.right = bestBidiRect.left + 1;
                                    }
                                    hasBestBidiRect = true;
                                    nearestForwardSrcIndex = word->src_text_index;
                                    nearestForwardSrcOffset = word->u.t.start;
                                }
                            } else if (word->u.t.start + word->u.t.len <= offset) { // past word in logical order
                                // Only if/while we haven't yet found one with the right src index and
                                // a forward offset
                                if (nearestForwardSrcIndex != word->src_text_index ||
                                    (nearestForwardSrcOffset < word->u.t.start &&
                                     word->u.t.start + word->u.t.len > nearestForwardSrcOffset)) {
                                    bestBidiRect.top = rc.top + frmline->y;
                                    bestBidiRect.bottom = bestBidiRect.top + frmline->height;
                                    if (word_is_rtl) { // left edge of previous logical word, as it is drawn on the right
                                        bestBidiRect.left = word->x + rc.left + frmline->x;
                                        bestBidiRect.right = bestBidiRect.left + 1;
                                    } else { // right edge of previous logical word, as it is drawn on the left
                                        bestBidiRect.right = word->x + word->width + rc.left + frmline->x;
                                        bestBidiRect.left = bestBidiRect.right - 1;
                                    }
                                    hasBestBidiRect = true;
                                    nearestForwardSrcIndex = word->src_text_index;
                                    nearestForwardSrcOffset = word->u.t.start + word->u.t.len;
                                }
                            } else { // exact word found
                                // Measure word
                                LVFont* font = (LVFont*)txtform->GetSrcInfo(srcIndex)->u.t.font;
                                lUInt16 w[512];
                                lUInt8 flg[512];
                                lString32 str = node->getText();
                                if (offset == word->u.t.start && str.empty()) {
                                    rect.left = word->x + rc.left + frmline->x;
                                    rect.top = rc.top + frmline->y;
                                    rect.right = rect.left + 1;
                                    rect.bottom = rect.top + frmline->height;
                                    return true;
                                }
                                // We need to transform the node text as it had been when
                                // rendered (the transform may change chars widths) for the
                                // rect to be correct
                                switch (node->getParentNode()->getStyle()->text_transform) {
                                    case css_tt_uppercase:
                                        str.uppercase();
                                        break;
                                    case css_tt_lowercase:
                                        str.lowercase();
                                        break;
                                    case css_tt_capitalize:
                                        str.capitalize();
                                        break;
                                    case css_tt_full_width:
                                        // str.fullWidthChars(); // disabled for now in lvrend.cpp
                                        break;
                                    default:
                                        break;
                                }
                                lUInt32 hints = WORD_FLAGS_TO_FNT_FLAGS(word->flags);
                                font->measureText(
                                        str.c_str() + word->u.t.start,
                                        word->u.t.len,
                                        w,
                                        flg,
                                        word->width + 50,
                                        '?',
                                        txtform->GetSrcInfo(srcIndex)->lang_cfg,
                                        txtform->GetSrcInfo(srcIndex)->letter_spacing + word->added_letter_spacing,
                                        false,
                                        hints);
                                rect.top = rc.top + frmline->y;
                                rect.bottom = rect.top + frmline->height;
                                // chx is the width of previous chars in the word
                                int chx = (offset > word->u.t.start) ? w[offset - word->u.t.start - 1] : 0;
                                if (word_is_rtl) {
                                    rect.right = word->x + word->width - chx + rc.left + frmline->x;
                                    rect.left = rect.right - 1;
                                } else {
                                    rect.left = word->x + chx + rc.left + frmline->x;
                                    rect.right = rect.left + 1;
                                }
                                if (extended) { // get width of char at offset
                                    if (offset == word->u.t.start && word->u.t.len == 1) {
                                        // With CJK chars, the measured width seems
                                        // less correct than the one measured while
                                        // making words. So use the calculated word
                                        // width for one-char-long words instead
                                        if (word_is_rtl)
                                            rect.left = rect.right - word->width;
                                        else
                                            rect.right = rect.left + word->width;
                                    } else {
                                        int chw = w[offset - word->u.t.start] - chx;
                                        bool hyphen_added = false;
                                        if (offset == word->u.t.start + word->u.t.len - 1 && (word->flags & LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER)) {
                                            // if offset is the end of word, and this word has
                                            // been hyphenated, includes the hyphen width
                                            chw += font->getHyphenWidth();
                                            // We then should not account for the right side
                                            // bearing below
                                            hyphen_added = true;
                                        }
                                        if (word_is_rtl) {
                                            rect.left = rect.right - chw;
                                            if (!hyphen_added) {
                                                // Also remove our added letter spacing for justification
                                                // from the left, to have cleaner highlights.
                                                rect.left += word->added_letter_spacing;
                                            }
                                        } else {
                                            rect.right = rect.left + chw;
                                            if (!hyphen_added) {
                                                // Also remove our added letter spacing for justification
                                                // from the right, to have cleaner highlights.
                                                rect.right -= word->added_letter_spacing;
                                            }
                                        }
                                        if (adjusted) {
                                            // Extend left or right if this glyph overflows its
                                            // origin/advance box (can happen with an italic font,
                                            // or with a regular font on the right of the letter 'f'
                                            // or on the left of the letter 'J').
                                            // Only when negative (overflow) and not when positive
                                            // (which are more frequent), mostly to keep some good
                                            // looking rectangles on the sides when highlighting
                                            // multiple lines.
                                            rect.left += font->getLeftSideBearing(str[offset], true);
                                            if (!hyphen_added)
                                                rect.right -= font->getRightSideBearing(str[offset], true);
                                            // Should work wheter rtl or ltr
                                        }
                                    }
                                    // Ensure we always return a non-zero width, even for zero-width
                                    // chars or collapsed spaces (to avoid isEmpty() returning true
                                    // which could be considered as a failure)
                                    if (rect.right <= rect.left) {
                                        if (word_is_rtl)
                                            rect.left = rect.right - 1;
                                        else
                                            rect.right = rect.left + 1;
                                    }
                                }
                                return true;
                            }
                        }
                        if (lastWord) {
                            // If no exact word found, return best candidate
                            if (hasBestBidiRect) {
                                rect = bestBidiRect;
                                return true;
                            }
                            // Otherwise, return end of last word (?)
                            rect.top = rc.top + frmline->y;
                            rect.bottom = rect.top + frmline->height;
                            rect.left = word->x + rc.left + frmline->x + word->width;
                            rect.right = rect.left + 1;
                            return true;
                        }
                    }
                    continue;
                } // end if line_is_bidi

                // ================================
                // Generic code when visual order = logical order
                if (word->src_text_index >= srcIndex || lastWord) {
                    // found word from same src line
                    if (word->flags & (LTEXT_WORD_IS_OBJECT | LTEXT_WORD_IS_INLINE_BOX) || word->src_text_index > srcIndex || (!extended && offset <= word->u.t.start) || (extended && offset < word->u.t.start)
                        // if extended, and offset = word->u.t.start, we want to
                        // measure the first char, which is done in the next else
                    ) {
                        // before this word
                        rect.left = word->x + rc.left + frmline->x;
                        //rect.top = word->y + rc.top + frmline->y + frmline->baseline;
                        rect.top = rc.top + frmline->y;
                        if (extended) {
                            if (word->flags & (LTEXT_WORD_IS_OBJECT | LTEXT_WORD_IS_INLINE_BOX) && word->width > 0)
                                rect.right = rect.left + word->width; // width of image
                            else
                                rect.right = rect.left + 1; // not the right word: no char width
                        } else {
                            rect.right = rect.left + 1;
                        }
                        rect.bottom = rect.top + frmline->height;
                        return true;
                    } else if ((word->src_text_index == srcIndex) &&
                               ((offset < word->u.t.start + word->u.t.len) ||
                                (offset == srcLen && offset == word->u.t.start + word->u.t.len))) {
                        // pointer inside this word
                        LVFont* font = (LVFont*)txtform->GetSrcInfo(srcIndex)->u.t.font;
                        lUInt16 w[512];
                        lUInt8 flg[512];
                        lString32 str = node->getText();
                        // With "|| (extended && offset < word->u.t.start)" added to the first if
                        // above, we may now be here with: offset = word->u.t.start = 0
                        // and a node->getText() returning THE lString32::empty_str:
                        // font->measureText() would segfault on it because its just a dummy
                        // pointer. Not really sure why that happens.
                        // It happens when node is the <a> in:
                        //     <div><span> <a id="someId"/>Anciens </span> <b>...
                        // and offset=0, word->u.t.start=0, word->u.t.len=8 .
                        // We can just do as in the first 'if'.
                        if (offset == word->u.t.start && str.empty()) {
                            rect.left = word->x + rc.left + frmline->x;
                            rect.top = rc.top + frmline->y;
                            rect.right = rect.left + 1;
                            rect.bottom = rect.top + frmline->height;
                            return true;
                        }
                        // We need to transform the node text as it had been when
                        // rendered (the transform may change chars widths) for the
                        // rect to be correct
                        switch (node->getParentNode()->getStyle()->text_transform) {
                            case css_tt_uppercase:
                                str.uppercase();
                                break;
                            case css_tt_lowercase:
                                str.lowercase();
                                break;
                            case css_tt_capitalize:
                                str.capitalize();
                                break;
                            case css_tt_full_width:
                                // str.fullWidthChars(); // disabled for now in lvrend.cpp
                                break;
                            default:
                                break;
                        }
                        lUInt32 hints = WORD_FLAGS_TO_FNT_FLAGS(word->flags);
                        font->measureText(
                                str.c_str() + word->u.t.start,
                                word->u.t.len,
                                w,
                                flg,
                                word->width + 50,
                                '?',
                                txtform->GetSrcInfo(srcIndex)->lang_cfg,
                                txtform->GetSrcInfo(srcIndex)->letter_spacing + word->added_letter_spacing,
                                false,
                                hints);
                        // chx is the width of previous chars in the word
                        int chx = (offset > word->u.t.start) ? w[offset - word->u.t.start - 1] : 0;
                        rect.left = word->x + chx + rc.left + frmline->x;
                        //rect.top = word->y + rc.top + frmline->y + frmline->baseline;
                        rect.top = rc.top + frmline->y;
                        if (extended) { // get width of char at offset
                            if (offset == word->u.t.start && word->u.t.len == 1) {
                                // With CJK chars, the measured width seems
                                // less correct than the one measured while
                                // making words. So use the calculated word
                                // width for one-char-long words instead
                                rect.right = rect.left + word->width;
                            } else {
                                int chw = w[offset - word->u.t.start] - chx;
                                bool hyphen_added = false;
                                if (offset == word->u.t.start + word->u.t.len - 1 && (word->flags & LTEXT_WORD_CAN_HYPH_BREAK_LINE_AFTER)) {
                                    // if offset is the end of word, and this word has
                                    // been hyphenated, includes the hyphen width
                                    chw += font->getHyphenWidth();
                                    // We then should not account for the right side
                                    // bearing below
                                    hyphen_added = true;
                                }
                                rect.right = rect.left + chw;
                                if (!hyphen_added) {
                                    // Also remove our added letter spacing for justification
                                    // from the right, to have cleaner highlights.
                                    rect.right -= word->added_letter_spacing;
                                }
                                if (adjusted) {
                                    // Extend left or right if this glyph overflows its
                                    // origin/advance box (can happen with an italic font,
                                    // or with a regular font on the right of the letter 'f'
                                    // or on the left of the letter 'J').
                                    // Only when negative (overflow) and not when positive
                                    // (which are more frequent), mostly to keep some good
                                    // looking rectangles on the sides when highlighting
                                    // multiple lines.
                                    rect.left += font->getLeftSideBearing(str[offset], true);
                                    if (!hyphen_added)
                                        rect.right -= font->getRightSideBearing(str[offset], true);
                                }
                            }
                            // Ensure we always return a non-zero width, even for zero-width
                            // chars or collapsed spaces (to avoid isEmpty() returning true
                            // which could be considered as a failure)
                            if (rect.right <= rect.left)
                                rect.right = rect.left + 1;
                        } else
                            rect.right = rect.left + 1;
                        rect.bottom = rect.top + frmline->height;
                        return true;
                    } else if (lastWord) {
                        // after last word
                        rect.left = word->x + rc.left + frmline->x + word->width;
                        //rect.top = word->y + rc.top + frmline->y + frmline->baseline;
                        rect.top = rc.top + frmline->y;
                        if (extended)
                            rect.right = rect.left + 1; // not the right word: no char width
                        else
                            rect.right = rect.left + 1;
                        rect.bottom = rect.top + frmline->height;
                        return true;
                    }
                }
            }
        }
        // return false;
        // Not found, which is possible with a final node with only empty
        // elements. This final node has a rect, so use it.
        rect = rc;
        return true;
    } else {
        // no base final node, using blocks
        //lvRect rc;
        ldomNode* node = getNode();
        int offset = getOffset();
        if (offset < 0 || node->getChildCount() == 0) {
            node->getAbsRect(rect);
            return true;
            //return rc.topLeft();
        }
        if (offset < (int)node->getChildCount()) {
            node->getChildNode(offset)->getAbsRect(rect);
            return true;
            //return rc.topLeft();
        }
        node->getChildNode(node->getChildCount() - 1)->getAbsRect(rect);
        return true;
        //return rc.bottomRight();
    }
}

lString32 ldomXPointer::toString(XPointerMode mode) const {
    if (XPATH_USE_NAMES == mode) {
        tinyNodeCollection* doc = (tinyNodeCollection*)_data->getDocument();
        if (doc != NULL && doc->getDOMVersionRequested() >= DOM_VERSION_WITH_NORMALIZED_XPOINTERS)
            return toStringV2();
        return toStringV1();
    }
    return toStringV2AsIndexes();
}

// Using names, old, with boxing elements (non-normalized)
lString32 ldomXPointer::toStringV1() const {
    lString32 path;
    if (isNull())
        return path;
    ldomNode* node = getNode();
    int offset = getOffset();
    if (offset >= 0) {
        path << "." << fmt::decimal(offset);
    }
    ldomNode* p = node;
    ldomNode* mainNode = node->getDocument()->getRootNode();
    while (p && p != mainNode) {
        ldomNode* parent = p->getParentNode();
        if (p->isElement()) {
            // element
            lString32 name = p->getNodeName();
            lUInt16 id = p->getNodeId();
            if (!parent)
                return "/" + name + path;
            int index = -1;
            int count = 0;
            for (int i = 0; i < parent->getChildCount(); i++) {
                ldomNode* node = parent->getChildElementNode(i, id);
                if (node) {
                    count++;
                    if (node == p)
                        index = count;
                }
            }
            if (count > 1)
                path = cs32("/") + name + "[" + fmt::decimal(index) + "]" + path;
            else
                path = cs32("/") + name + path;
        } else {
            // text
            if (!parent)
                return cs32("/text()") + path;
            int index = -1;
            int count = 0;
            for (int i = 0; i < parent->getChildCount(); i++) {
                ldomNode* node = parent->getChildNode(i);
                if (node->isText()) {
                    count++;
                    if (node == p)
                        index = count;
                }
            }
            if (count > 1)
                path = cs32("/text()") + "[" + fmt::decimal(index) + "]" + path;
            else
                path = "/text()" + path;
        }
        p = parent;
    }
    return path;
}

template <typename T>
static int getElementIndex(ldomNode* parent, ldomNode* targetNode, T predicat, int& count) {
    for (int i = 0; i < parent->getChildCount(); i++) {
        ldomNode* node = parent->getChildNode(i);
        if (isBoxingNode(node) && targetNode != node) {
            int index = getElementIndex(node, targetNode, predicat, count);
            if (index > 0)
                return index;
        } else if (predicat(node))
            count++;
        if (node == targetNode)
            return count;
    }
    return -1;
}

// Using names, new, without boxing elements, so: normalized
lString32 ldomXPointer::toStringV2() const {
    lString32 path;
    if (isNull())
        return path;
    ldomNode* node = getNode();
    int offset = getOffset();
    ldomNode* p = node;
    if (!node->isBoxingNode(true)) { // (nor pseudoElem)
        if (offset >= 0) {
            path << "." << fmt::decimal(offset);
        }
    } else {
        // Be really sure we get a non boxing node
        if (offset < p->getChildCount()) {
            p = p->getChildNode(offset);
            if (p->isBoxingNode(true)) {
                p = p->getUnboxedFirstChild();
                if (!p)
                    p = node->getUnboxedParent();
            }
        } else {
            p = p->getUnboxedParent();
        }
    }
    ldomNode* mainNode = node->getDocument()->getRootNode();
    while (p && p != mainNode) {
        ldomNode* parent = p->getParentNode();
        while (isBoxingNode(parent))
            parent = parent->getParentNode();
        if (p->isElement()) {
            // element
            lString32 name = p->getNodeName();
            if (!parent)
                return "/" + name + path;
            int count = 0;
            ldomNodeIdPredicate predicat(p->getNodeId());
            int index = getElementIndex(parent, p, predicat, count);
            if (count == 1) {
                // We're first, but see if we have following siblings with the
                // same element name, so we can have "div[1]" instead of "div"
                // when parent has more than one of it (as toStringV1 does).
                ldomNode* n = p;
                while ((n = n->getUnboxedNextSibling(true))) {
                    if (predicat(n)) { // We have such a followup sibling
                        count = 2;     // there's at least 2 of them
                        break;
                    }
                }
            }
            if (count > 1)
                path = cs32("/") + name + "[" + fmt::decimal(index) + "]" + path;
            else
                path = cs32("/") + name + path;
        } else {
            // text
            if (!parent)
                return cs32("/text()") + path;
            int count = 0;
            int index = getElementIndex(parent, p, isTextNode, count);
            if (count == 1) {
                // We're first, but see if we have following text siblings,
                // so we can have "text()[1]" instead of "text()" when
                // parent has more than one text node (as toStringV1 does).
                ldomNode* n = p;
                while ((n = n->getUnboxedNextSibling(false))) {
                    if (isTextNode(n)) { // We have such a followup sibling
                        count = 2;       // there's at least 2 of them
                        break;
                    }
                }
            }
            if (count > 1)
                path = cs32("/text()") + "[" + fmt::decimal(index) + "]" + path;
            else
                path = "/text()" + path;
        }
        p = parent;
    }
    return path;
}

// Without element names, normalized (not used)
lString32 ldomXPointer::toStringV2AsIndexes() const {
    lString32 path;
    if (isNull())
        return path;
    int offset = getOffset();
    if (offset >= 0) {
        path << "." << fmt::decimal(offset);
    }
    ldomNode* p = getNode();
    ldomNode* rootNode = p->getDocument()->getRootNode();
    while (p && p != rootNode) {
        ldomNode* parent = p->getParentNode();
        if (!parent)
            return "/" + (p->isElement() ? p->getNodeName() : cs32("/text()")) + path;

        while (isBoxingNode(parent))
            parent = parent->getParentNode();

        int count = 0;
        int index = getElementIndex(parent, p, notNull, count);

        if (index > 0) {
            path = cs32("/") + fmt::decimal(index) + path;
        } else {
            CRLog::error("!!! child node not found in a parent");
        }
        p = parent;
    }
    return path;
}

/// returns HTML (serialized from the DOM, may be different from the source HTML)
/// puts the paths of the linked css files met into the provided lString32Collection cssFiles
lString8 ldomXPointer::getHtml(lString32Collection& cssFiles, int wflags) const {
    if (isNull())
        return lString8::empty_str;
    ldomNode* startNode = getNode();
    LVStreamRef stream = LVCreateMemoryStream(NULL, 0, false, LVOM_WRITE);
    writeNodeEx(stream.get(), startNode, cssFiles, wflags);
    int size = stream->GetSize();
    LVArray<char> buf(size + 1, '\0');
    stream->Seek(0, LVSEEK_SET, NULL);
    stream->Read(buf.get(), size, NULL);
    buf[size] = 0;
    lString8 html = lString8(buf.get());
    return html;
}

/// returns href attribute of <A> element, null string if not found
lString32 ldomXPointer::getHRef() const {
    ldomXPointer unused_a_xpointer;
    return getHRef(unused_a_xpointer);
}

/// returns href attribute of <A> element, plus xpointer of <A> element itself
lString32 ldomXPointer::getHRef(ldomXPointer& a_xpointer) const {
    if (isNull())
        return lString32::empty_str;
    ldomNode* node = getNode();
    while (node && !node->isElement())
        node = node->getParentNode();
    while (node && node->getNodeId() != el_a)
        node = node->getParentNode();
    if (!node)
        return lString32::empty_str;
    a_xpointer.setNode(node);
    a_xpointer.setOffset(0);
    lString32 ref = node->getAttributeValue(LXML_NS_ANY, attr_href);
    if (!ref.empty() && ref[0] != '#')
        ref = DecodeHTMLUrlString(ref);
    return ref;
}
