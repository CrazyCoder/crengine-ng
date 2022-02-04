/**
 * CoolReader Engine
 *
 * (c) poire-z <poire-z@users.noreply.github.com>, 2018
 *
 * This source code is distributed under the terms of
 * GNU General Public License version 2
 * See LICENSE file for details
 */

#ifndef __WRITENODEEX_H_INCLUDED__
#define __WRITENODEEX_H_INCLUDED__

#include <ldomxpointerex.h>

// Extended version of previous function for displaying selection HTML, with tunable output
#define WRITENODEEX_TEXT_HYPHENATE              0x0001 ///< add soft-hyphens where hyphenation is allowed
#define WRITENODEEX_TEXT_MARK_NODE_BOUNDARIES   0x0002 ///< mark start and end of text nodes (useful when indented)
#define WRITENODEEX_TEXT_SHOW_UNICODE_CODEPOINT 0x0004 ///< show unicode codepoint after char
#define WRITENODEEX_TEXT_UNESCAPED              0x0008 ///< let &, < and > unescaped in text nodes (makes HTML invalid)
#define WRITENODEEX_INDENT_NEWLINE              0x0010 ///< indent newlines according to node level
#define WRITENODEEX_NEWLINE_BLOCK_NODES         0x0020 ///< start only nodes rendered as block/final on a new line, \ \
        ///  so inline elements and text nodes are stuck together
#define WRITENODEEX_NEWLINE_ALL_NODES           0x0040 ///< start all nodes on a new line
#define WRITENODEEX_UNUSED_1                    0x0080 ///<
#define WRITENODEEX_NB_SKIPPED_CHARS            0x0100 ///< show number of skipped chars in text nodes: (...43...)
#define WRITENODEEX_NB_SKIPPED_NODES            0x0200 ///< show number of skipped sibling nodes: [...17...]
#define WRITENODEEX_SHOW_REND_METHOD            0x0400 ///< show rendering method at end of tag (<div ~F> =Final, <b ~i>=Inline...)
#define WRITENODEEX_SHOW_MISC_INFO              0x0800 ///< show additional info (depend on context)
#define WRITENODEEX_ADD_UPPER_DIR_LANG_ATTR     0x1000 ///< add dir= and lang= grabbed from upper nodes
#define WRITENODEEX_GET_CSS_FILES               0x2000 ///< ensure css files that apply to initial node are returned \ \
        ///  in &cssFiles (needed when not starting from root node)
#define WRITENODEEX_INCLUDE_STYLESHEET_ELEMENT  0x4000 ///< includes crengine <stylesheet> element in HTML \ \
        ///  (not done if outside of sub-tree)
#define WRITENODEEX_COMPUTED_STYLES_AS_ATTR     0x8000 ///< set style='' from computed styles (not implemented)

#define WNEFLAG(x) (wflags & WRITENODEEX_##x)

void writeNodeEx(LVStream* stream, ldomNode* node, lString32Collection& cssFiles, int wflags = 0,
                 ldomXPointerEx startXP = ldomXPointerEx(), ldomXPointerEx endXP = ldomXPointerEx(), int indentBaseLevel = -1);

#endif // __WRITENODEEX_H_INCLUDED__
