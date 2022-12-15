/***************************************************************************
 *   crengine-ng                                                           *
 *   Copyright (C) 2007,2009,2010,2012 Vadim Lopatin <coolreader.org@gmail.com>
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

#ifndef __LDOMTEXTCOLLECTOR_H_INCLUDED__
#define __LDOMTEXTCOLLECTOR_H_INCLUDED__

#include <ldomnodecallback.h>
#include <ldomxrange.h>

class ldomTextCollector: public ldomNodeCallback
{
private:
    bool lastText;
    bool newBlock;
    lChar32 delimiter;
    int maxLen;
    lString32 text;
public:
    ldomTextCollector(lChar32 blockDelimiter, int maxTextLen)
            : lastText(false)
            , newBlock(true)
            , delimiter(blockDelimiter)
            , maxLen(maxTextLen) {
    }
    /// destructor
    virtual ~ldomTextCollector() { }
    /// called for each found text fragment in range
    virtual void onText(ldomXRange* nodeRange) {
        if (newBlock && !text.empty()) {
            text << delimiter;
        }
        lString32 txt = nodeRange->getStart().getNode()->getText();
        int start = nodeRange->getStart().getOffset();
        int end = nodeRange->getEnd().getOffset();
        if (start < end) {
            text << txt.substr(start, end - start);
        }
        lastText = true;
        newBlock = false;
    }
    /// called for each found node in range
    virtual bool onElement(ldomXPointerEx* ptr) {
        ldomNode* elem = (ldomNode*)ptr->getNode();
        // Allow tweaking that with hints
        css_style_ref_t style = elem->getStyle();
        if (STYLE_HAS_CR_HINT(style, TEXT_SELECTION_SKIP)) {
            return false;
        } else if (STYLE_HAS_CR_HINT(style, TEXT_SELECTION_INLINE)) {
            newBlock = false;
            return true;
        } else if (STYLE_HAS_CR_HINT(style, TEXT_SELECTION_BLOCK)) {
            newBlock = true;
            return true;
        }
        lvdom_element_render_method rm = elem->getRendMethod();
        if (rm == erm_invisible)
            return false;
        if (rm == erm_inline) {
            // Don't set newBlock if rendering method is erm_inline,
            // no matter the original CSS display.
            // (Don't reset any previously set and not consumed newBlock)
            return true;
        }
        // For other rendering methods (that would bring newBlock=true),
        // look at the initial CSS display, as we might have boxed some
        // inline-like elements for rendering purpose.
        css_display_t d = style->display;
        if (d <= css_d_inline || d == css_d_inline_block || d == css_d_inline_table) {
            // inline, ruby; consider inline-block/-table as inline, in case
            // they don't contain much (if they do, some inner block element
            // will set newBlock=true).
            return true;
        }
        // Otherwise, it's a block like node, and we want a \n before the next text
        newBlock = true;
        return true;
    }
    /// get collected text
    lString32 getText() {
        return text;
    }
};

#endif // __LDOMTEXTCOLLECTOR_H_INCLUDED__
