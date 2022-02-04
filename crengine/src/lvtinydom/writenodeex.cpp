/**
 * CoolReader Engine
 *
 * poire-z <poire-z@users.noreply.github.com>, 2018
 *
 * This source code is distributed under the terms of
 * GNU General Public License version 2
 * See LICENSE file for details
 */

#include "writenodeex.h"

#include <ldomdocument.h>
#include <lvstsheet.h>
#include <fb2def.h>
#include <lvstreamutils.h>

#include "lxmlattribute.h"
#include "../textlang.h"

void writeNodeEx(LVStream* stream, ldomNode* node, lString32Collection& cssFiles, int wflags,
                 ldomXPointerEx startXP, ldomXPointerEx endXP, int indentBaseLevel) {
    bool isStartNode = false;
    bool isEndNode = false;
    bool isAfterStart = false;
    bool isBeforeEnd = false;
    bool containsStart = false;
    bool containsEnd = false;

    if (!startXP.isNull() && !endXP.isNull()) {
        ldomXPointerEx currentEXP = ldomXPointerEx(node, 0);
        // Use start (offset=0) of text node for comparisons, but keep original XPointers
        ldomXPointerEx startEXP = ldomXPointerEx(startXP);
        startEXP.setOffset(0);
        ldomXPointerEx endEXP = ldomXPointerEx(endXP);
        endEXP.setOffset(0);
        if (currentEXP == startEXP)
            isStartNode = true;
        if (currentEXP == endEXP)
            isEndNode = true;
        if (currentEXP.compare(startEXP) >= 0) {
            isAfterStart = true;
        }
        if (currentEXP.compare(endEXP) <= 0) {
            isBeforeEnd = true;
        }
        ldomNode* tmp;
        tmp = startXP.getNode();
        while (tmp) {
            if (tmp == node) {
                containsStart = true;
                break;
            }
            tmp = tmp->getParentNode();
        }
        tmp = endXP.getNode();
        while (tmp) {
            if (tmp == node) {
                containsEnd = true;
                break;
            }
            tmp = tmp->getParentNode();
        }
    } else {
        containsStart = true;
        containsEnd = true;
        isAfterStart = true;
        isBeforeEnd = true;
        // but not isStartNode nor isEndNode, as these use startXP and endXP
    }

    bool isInitialNode = false;
    lString32 initialDirAttribute = lString32::empty_str;
    lString32 initialLangAttribute = lString32::empty_str;
    if (indentBaseLevel < 0) { // initial call (recursive ones will have it >=0)
        indentBaseLevel = node->getNodeLevel();
        isInitialNode = true;
        if (WNEFLAG(ADD_UPPER_DIR_LANG_ATTR) && !node->isRoot()) {
            // Grab any dir="rtl" and lang="ar_AA" attributes from some parent node
            if (!node->hasAttribute(attr_dir)) {
                ldomNode* pnode = node->getParentNode();
                for (; pnode && !pnode->isNull() && !pnode->isRoot(); pnode = pnode->getParentNode()) {
                    if (pnode->hasAttribute(attr_dir)) {
                        initialDirAttribute = pnode->getAttributeValue(attr_dir);
                        break;
                    }
                }
            }
            if (!node->hasAttribute(attr_lang)) {
                ldomNode* pnode = node->getParentNode();
                for (; pnode && !pnode->isNull() && !pnode->isRoot(); pnode = pnode->getParentNode()) {
                    if (pnode->hasAttribute(attr_lang)) {
                        initialLangAttribute = pnode->getAttributeValue(attr_lang);
                        break;
                    }
                }
            }
        }
    }
    int level = node->getNodeLevel();
    if (node->isText() && isAfterStart && isBeforeEnd) {
        bool doNewLine = WNEFLAG(NEWLINE_ALL_NODES);
        bool doIndent = doNewLine && WNEFLAG(INDENT_NEWLINE);
        lString32 txt = node->getText();
        lString8 prefix = lString8::empty_str;
        lString8 suffix = lString8::empty_str;

        if (isEndNode) {
            // show the number of chars not written after selection "(...n...)"
            int nodeLength = endXP.getText().length();
            int endOffset = endXP.getOffset();
            if (endOffset < nodeLength) {
                txt = txt.substr(0, endOffset);
                if (WNEFLAG(NB_SKIPPED_CHARS))
                    suffix << "(…" << lString8().appendDecimal(nodeLength - endOffset) << "…)";
            }
        }
        if (WNEFLAG(TEXT_MARK_NODE_BOUNDARIES)) {
            // We use non-ordinary chars to mark start and end of text
            // node, which can help noticing spaces at start or end
            // when NEWLINE_ALL_NODES and INDENT_NEWLINE are used.
            // Some candidates chars are:
            //   Greyish, discreet, but may be confused with parenthesis:
            //     prefix << "⟨"; // U+27E8 Mathematical Left Angle Bracket
            //     suffix << "⟩"; // U+27E9 Mathematical Right Angle Bracket
            //   Greyish, a bit less discreet, but won't be confused with any other casual char:
            //     prefix << "⟪"; // U+27EA Mathematical Left Double Angle Bracket
            //     suffix << "⟫"; // U+27EB Mathematical Right Double Angle Bracket
            //   A bit too dark, but won't be confused with any other casual char:
            //     prefix << "⎛"; // U+239B Left Parenthesis Upper Hook
            //     suffix << "⎠"; // U+23A0 Right Parenthesis Lower Hook (may have too much leading space)
            prefix << "⟪"; // U+27EA Mathematical Left Double Angle Bracket
            suffix << "⟫"; // U+27EB Mathematical Right Double Angle Bracket
        }
        if (isStartNode) {
            // show the number of chars not written before selection "(...n...)"
            int offset = startXP.getOffset();
            if (offset > 0) {
                txt = txt.substr(offset);
                if (WNEFLAG(NB_SKIPPED_CHARS))
                    prefix << "(…" << lString8().appendDecimal(offset) << "…)";
            }
            if (WNEFLAG(NB_SKIPPED_NODES)) {
                // show the number of sibling nodes not written before selection "[...n..]"
                int nbIgnoredPrecedingSiblings = node->getNodeIndex();
                if (nbIgnoredPrecedingSiblings) {
                    if (doIndent)
                        for (int i = indentBaseLevel; i < level; i++)
                            *stream << "  ";
                    *stream << "[…" << lString8().appendDecimal(nbIgnoredPrecedingSiblings) << "…]";
                    if (doNewLine)
                        *stream << "\n";
                }
            }
        }
        if (doIndent)
            for (int i = indentBaseLevel; i < level; i++)
                *stream << "  ";
        if (!WNEFLAG(TEXT_UNESCAPED)) {
            // Use a temporary char we're not likely to find in the DOM
            // (see https://en.wikipedia.org/wiki/Specials_(Unicode_block) )
            // for 2-steps '&' replacement (to avoid infinite loop or the
            // need for more complicated code)
            while (txt.replace(cs32("&"), cs32(U"\xFFFF")))
                ;
            while (txt.replace(cs32(U"\xFFFF"), cs32("&amp;")))
                ;
            while (txt.replace(cs32("<"), cs32("&lt;")))
                ;
            while (txt.replace(cs32(">"), cs32("&gt;")))
                ;
        }
#define HYPH_MIN_WORD_LEN_TO_HYPHENATE 4
#define HYPH_MAX_WORD_SIZE             64
        // (No hyphenation if we are showing unicode codepoint)
        if (WNEFLAG(TEXT_SHOW_UNICODE_CODEPOINT)) {
            *stream << prefix;
            for (int i = 0; i < txt.length(); i++)
                *stream << UnicodeToUtf8(txt.substr(i, 1)) << "⟨U+" << lString8().appendHex(txt[i]) << "⟩";
            *stream << suffix;
        } else if (WNEFLAG(TEXT_HYPHENATE) && HyphMan::isEnabled() && txt.length() >= HYPH_MIN_WORD_LEN_TO_HYPHENATE) {
            // Add soft-hyphens where HyphMan (with the user or language current hyphenation
            // settings) says hyphenation is allowed.
            // We do that here while we output the text to avoid the need
            // for temporary storage of a string with soft-hyphens added.
            const lChar32* text32 = txt.c_str();
            int txtlen = txt.length();
            lUInt8* flags = (lUInt8*)calloc(txtlen, sizeof(*flags));
            lUInt16 widths[HYPH_MAX_WORD_SIZE] = { 0 }; // array needed by hyphenate()
            // Lookup words starting from the end, just because lStr_findWordBounds()
            // will ensure the iteration that way.
            int wordpos = txtlen;
            while (wordpos > 0) {
                // lStr_findWordBounds() will find the word contained at wordpos
                // (or the previous word if wordpos happens to be a space or some
                // punctuation) by looking only for alpha chars in m_text.
                int start, end;
                bool has_rtl;
                lStr_findWordBounds(text32, txtlen, wordpos, start, end, has_rtl);
                if (end <= HYPH_MIN_WORD_LEN_TO_HYPHENATE) {
                    // Too short word at start, we're done
                    break;
                }
                int len = end - start;
                if (len < HYPH_MIN_WORD_LEN_TO_HYPHENATE || has_rtl) {
                    // Too short word found, or word containing RTL: skip it
                    wordpos = start - 1;
                    continue;
                }
                if (start >= wordpos) {
                    // Shouldn't happen, but let's be sure we don't get stuck
                    wordpos = wordpos - HYPH_MIN_WORD_LEN_TO_HYPHENATE;
                    continue;
                }
                // We have a valid word to look for hyphenation
                if (len > HYPH_MAX_WORD_SIZE) // hyphenate() stops/truncates at 64 chars
                    len = HYPH_MAX_WORD_SIZE;
                // Have hyphenate() set flags inside 'flags'
                // (Fetching the lang_cfg for each text node is not really cheap, but
                // it's easier than having to pass it to each writeNodeEx())
                TextLangMan::getTextLangCfg(node)->getHyphMethod()->hyphenate(text32 + start, len, widths, flags + start, 0, 0xFFFF, 1);
                // Continue with previous word
                wordpos = start - 1;
            }
            // Output text, and add a soft-hyphen where there are flags
            *stream << prefix;
            for (int i = 0; i < txt.length(); i++) {
                *stream << UnicodeToUtf8(txt.substr(i, 1));
                if (flags[i] & LCHAR_ALLOW_HYPH_WRAP_AFTER)
                    *stream << "­";
            }
            *stream << suffix;
            free(flags);
        } else {
            *stream << prefix << UnicodeToUtf8(txt) << suffix;
        }
        if (doNewLine)
            *stream << "\n";
        if (isEndNode && WNEFLAG(NB_SKIPPED_NODES)) {
            // show the number of sibling nodes not written after selection "[...n..]"
            ldomNode* parent = node->getParentNode();
            int nbIgnoredFollowingSiblings = parent ? (parent->getChildCount() - 1 - node->getNodeIndex()) : 0;
            if (nbIgnoredFollowingSiblings) {
                if (doIndent)
                    for (int i = indentBaseLevel; i < level; i++)
                        *stream << "  ";
                *stream << "[…" << lString8().appendDecimal(nbIgnoredFollowingSiblings) << "…]";
                if (doNewLine)
                    *stream << "\n";
            }
        }
    } else if (node->isElement()) {
        lString8 elemName = UnicodeToUtf8(node->getNodeName());
        lString8 elemNsName = UnicodeToUtf8(node->getNodeNsName());
        // Write elements that are between start and end, but also those that
        // are parents of start and end nodes
        bool toWrite = (isAfterStart && isBeforeEnd) || containsStart || containsEnd;
        bool isStylesheetTag = false;
        if (node->getNodeId() == el_stylesheet) {
            toWrite = false;
            if (WNEFLAG(INCLUDE_STYLESHEET_ELEMENT)) {
                // We may meet a <stylesheet> tag that is not between startXP and endXP and
                // does not contain any of them, but its parent (body or DocFragment) does.
                // Write it if requested, as it's useful when inspecting HTML.
                toWrite = true;
                isStylesheetTag = true; // for specific parsing and writting
            }
        }
        if (!toWrite)
            return;

        // In case we're called (when debugging) while styles have been reset,
        // avoid crash on stuff like isBoxingInlineBox()/isFloatingBox() that
        // do check styles
        bool has_styles_set = !node->getStyle().isNull();

        bool doNewLineBeforeStartTag = false;
        bool doNewLineAfterStartTag = false;
        bool doNewLineBeforeEndTag = false; // always stays false, newline done by child elements
        bool doNewLineAfterEndTag = false;
        bool doIndentBeforeStartTag = false;
        bool doIndentBeforeEndTag = false;
        // Specific for floats and inline-blocks among inlines inside final, that
        // we want to show on their own lines:
        bool doNewlineBeforeIndentBeforeStartTag = false;
        bool doIndentAfterNewLineAfterEndTag = false;
        bool doIndentOneLevelLessAfterNewLineAfterEndTag = false;
        if (WNEFLAG(NEWLINE_ALL_NODES)) {
            doNewLineBeforeStartTag = true;
            doNewLineAfterStartTag = true;
            // doNewLineBeforeEndTag = false; // done by child elements
            doNewLineAfterEndTag = true;
            doIndentBeforeStartTag = WNEFLAG(INDENT_NEWLINE);
            doIndentBeforeEndTag = WNEFLAG(INDENT_NEWLINE);
        } else if (WNEFLAG(NEWLINE_BLOCK_NODES)) {
            // We consider block elements according to crengine decision for their
            // rendering method, which gives us a visual hint of it.
            lvdom_element_render_method rm = node->getRendMethod();
            // Text and inline nodes stay stuck together, but not all others
            if (rm == erm_invisible) {
                // We don't know how invisible nodes would be displayed if
                // they were visible. Make the invisible tree like inline
                // among finals, so they don't take too much height.
                if (node->getParentNode()) {
                    rm = node->getParentNode()->getRendMethod();
                    if (rm == erm_invisible || rm == erm_inline || rm == erm_final)
                        rm = erm_inline;
                    else
                        rm = erm_final;
                }
            }
            if (rm != erm_inline || (has_styles_set && node->isBoxingInlineBox())) {
                doNewLineBeforeStartTag = true;
                doNewLineAfterStartTag = true;
                // doNewLineBeforeEndTag = false; // done by child elements
                doNewLineAfterEndTag = true;
                doIndentBeforeStartTag = WNEFLAG(INDENT_NEWLINE);
                doIndentBeforeEndTag = WNEFLAG(INDENT_NEWLINE);
                if (rm == erm_final) {
                    // Nodes with rend method erm_final contain only text and inline nodes.
                    // We want these erm_final indented, but not their content
                    doNewLineAfterStartTag = false;
                    doIndentBeforeEndTag = false;
                } else if (has_styles_set && node->isFloatingBox()) {
                    lvdom_element_render_method prm = node->getParentNode()->getRendMethod();
                    if (prm == erm_final || prm == erm_inline) {
                        doNewlineBeforeIndentBeforeStartTag = true;
                        doIndentAfterNewLineAfterEndTag = WNEFLAG(INDENT_NEWLINE);
                        // If we're the last node in parent collection, indent one level less,
                        // so that next node (the parent) is not at this node level
                        ldomNode* parent = node->getParentNode();
                        if (parent && (node->getNodeIndex() == parent->getChildCount() - 1))
                            doIndentOneLevelLessAfterNewLineAfterEndTag = true;
                        else if (parent && (node->getNodeIndex() == parent->getChildCount() - 2) && parent->getChildNode(parent->getChildCount() - 1)->isText())
                            doIndentOneLevelLessAfterNewLineAfterEndTag = true;
                        else if (containsEnd) // same if next siblings won't be shown
                            doIndentOneLevelLessAfterNewLineAfterEndTag = true;
                        // But if previous sibling node is a floating or boxing inline node
                        // that have done what we just did, cancel some of what we did
                        if (node->getNodeIndex() > 0) {
                            ldomNode* prevsibling = parent->getChildNode(node->getNodeIndex() - 1);
                            if (prevsibling->isFloatingBox() || prevsibling->isBoxingInlineBox()) {
                                doNewlineBeforeIndentBeforeStartTag = false;
                                doIndentBeforeStartTag = false;
                            }
                        }
                    }
                } else if (has_styles_set && node->isBoxingInlineBox()) {
                    doNewlineBeforeIndentBeforeStartTag = true;
                    doIndentAfterNewLineAfterEndTag = WNEFLAG(INDENT_NEWLINE);
                    // Same as above
                    ldomNode* parent = node->getParentNode();
                    if (parent && (node->getNodeIndex() == parent->getChildCount() - 1))
                        doIndentOneLevelLessAfterNewLineAfterEndTag = true;
                    else if (parent && (node->getNodeIndex() == parent->getChildCount() - 2) && parent->getChildNode(parent->getChildCount() - 1)->isText())
                        doIndentOneLevelLessAfterNewLineAfterEndTag = true;
                    else if (containsEnd)
                        doIndentOneLevelLessAfterNewLineAfterEndTag = true;
                    if (node->getNodeIndex() > 0) {
                        ldomNode* prevsibling = parent->getChildNode(node->getNodeIndex() - 1);
                        if (prevsibling->isFloatingBox() || prevsibling->isBoxingInlineBox()) {
                            doNewlineBeforeIndentBeforeStartTag = false;
                            doIndentBeforeStartTag = false;
                        }
                    }
                }
            }
        }

        if (containsStart && WNEFLAG(NB_SKIPPED_NODES)) {
            // Previous siblings did not contain startXP: show how many they are
            int nbIgnoredPrecedingSiblings = node->getNodeIndex();
            if (nbIgnoredPrecedingSiblings && WNEFLAG(INCLUDE_STYLESHEET_ELEMENT) &&
                node->getParentNode()->getFirstChild()->isElement() &&
                node->getParentNode()->getFirstChild()->getNodeId() == el_stylesheet) {
                nbIgnoredPrecedingSiblings--; // we have written the <stylesheet> tag
            }
            if (nbIgnoredPrecedingSiblings) {
                if (doIndentBeforeStartTag)
                    for (int i = indentBaseLevel; i < level; i++)
                        *stream << "  ";
                *stream << "[…" << lString8().appendDecimal(nbIgnoredPrecedingSiblings) << "…]";
                if (doNewLineBeforeStartTag)
                    *stream << "\n";
            }
        }
        if (doNewlineBeforeIndentBeforeStartTag)
            *stream << "\n";
        if (doIndentBeforeStartTag)
            for (int i = indentBaseLevel; i < level; i++)
                *stream << "  ";
        if (elemName.empty()) {
            // should not happen (except for the root node, that we might have skipped)
            elemName = node->isRoot() ? lString8("RootNode") : (elemNsName + "???");
        }
        if (!elemNsName.empty())
            elemName = elemNsName + ":" + elemName;
        *stream << "<" << elemName;
        if (isInitialNode) {
            // Add any dir="rtl" and lang="ar_AA" attributes grabbed from some parent node
            if (!initialDirAttribute.empty()) {
                *stream << " dir=\"" << UnicodeToUtf8(initialDirAttribute) << "\"";
            }
            if (!initialLangAttribute.empty()) {
                *stream << " lang=\"" << UnicodeToUtf8(initialLangAttribute) << "\"";
            }
        }
        for (int i = 0; i < (int)node->getAttrCount(); i++) {
            const lxmlAttribute* attr = node->getAttribute(i);
            if (attr) {
                lString8 attrName(UnicodeToUtf8(node->getDocument()->getAttrName(attr->id)));
                lString8 nsName(UnicodeToUtf8(node->getDocument()->getNsName(attr->nsid)));
                lString8 attrValue(UnicodeToUtf8(node->getDocument()->getAttrValue(attr->index)));
                if (WNEFLAG(SHOW_MISC_INFO) && has_styles_set) {
                    if (node->getNodeId() == el_pseudoElem && (attr->id == attr_Before || attr->id == attr_After)) {
                        // Show the rendered content as the otherwise empty Before/After attribute value
                        if (WNEFLAG(TEXT_SHOW_UNICODE_CODEPOINT)) {
                            lString32 content = get_applied_content_property(node);
                            attrValue.empty();
                            for (int i = 0; i < content.length(); i++) {
                                attrValue << UnicodeToUtf8(content.substr(i, 1)) << "⟨U+" << lString8().appendHex(content[i]) << "⟩";
                            }
                        } else {
                            attrValue = UnicodeToUtf8(get_applied_content_property(node));
                        }
                    }
                }
                *stream << " ";
                if (nsName.length() > 0)
                    *stream << nsName << ":";
                *stream << attrName;
                if (!attrValue.empty()) // don't show ="" if empty
                    *stream << "=\"" << attrValue << "\"";
                if (attrName == "StyleSheet") { // gather linked css files
                    lString32 cssFile = node->getDocument()->getAttrValue(attr->index);
                    if (!cssFiles.contains(cssFile))
                        cssFiles.add(cssFile);
                }
            }
        }
        if (WNEFLAG(SHOW_REND_METHOD)) {
            *stream << " ~";
            switch (node->getRendMethod()) {
                case erm_invisible:
                    *stream << "X";
                    break;
                case erm_killed:
                    *stream << "K";
                    break;
                case erm_block:
                    *stream << "B";
                    break;
                case erm_final:
                    *stream << "F";
                    break;
                case erm_inline:
                    *stream << "i";
                    break;
                case erm_table:
                    *stream << "T";
                    break;
                case erm_table_row_group:
                    *stream << "TRG";
                    break;
                case erm_table_header_group:
                    *stream << "THG";
                    break;
                case erm_table_footer_group:
                    *stream << "TFG";
                    break;
                case erm_table_row:
                    *stream << "TR";
                    break;
                case erm_table_column_group:
                    *stream << "TCG";
                    break;
                case erm_table_column:
                    *stream << "TC";
                    break;
                default:
                    *stream << "?";
                    break;
            }
        }
        if (node->getChildCount() == 0) {
            if (elemName[0] == '?')
                *stream << "?>";
            else
                *stream << "/>";
        } else {
            *stream << ">";
            if (doNewLineAfterStartTag)
                *stream << "\n";
            if (!isStylesheetTag) {
                for (int i = 0; i < (int)node->getChildCount(); i++) {
                    writeNodeEx(stream, node->getChildNode(i), cssFiles, wflags, startXP, endXP, indentBaseLevel);
                }
            } else {
                // We need to parse the stylesheet tag text to extract css files path.
                // We write its content without indentation and add a \n for readability.
                lString8 txt = node->getText8();
                int txtlen = txt.length();
                if (txtlen && txt.substr(txtlen - 1) != "\n") {
                    txt << "\n";
                }
                *stream << txt;
                // Parse @import'ed files to gather linked css files (we don't really need to
                // do recursive parsing of @import, which are very rare, we just want to get
                // the 2nd++ linked css files that were put there by crengine).
                const char* s = txt.c_str();
                while (true) {
                    lString8 import_file;
                    if (!LVProcessStyleSheetImport(s, import_file)) {
                        break;
                    }
                    lString32 cssFile = LVCombinePaths(node->getAttributeValue(attr_href), Utf8ToUnicode(import_file));
                    if (!cssFile.empty() && !cssFiles.contains(cssFile)) {
                        cssFiles.add(cssFile);
                    }
                }
            }
            if (doNewLineBeforeEndTag)
                *stream << "\n";
            if (doIndentBeforeEndTag)
                for (int i = indentBaseLevel; i < level; i++)
                    *stream << "  ";
            *stream << "</" << elemName << ">";
            if (WNEFLAG(TEXT_HYPHENATE)) {
                // Additional minor formatting tweaks for when this is going to be fed
                // to some other renderer, which is usually when we request HYPHENATE.
                if (has_styles_set && node->getStyle()->display == css_d_run_in) {
                    // For FB2 footnotes, add a space between the number and text,
                    // as none might be present in the source. If there were some,
                    // the other renderer will probably collapse them.
                    *stream << " ";
                }
            }
        }
        if (doNewLineAfterEndTag)
            *stream << "\n";
        if (doIndentAfterNewLineAfterEndTag) {
            int ilevel = doIndentOneLevelLessAfterNewLineAfterEndTag ? level - 1 : level;
            for (int i = indentBaseLevel; i < ilevel; i++)
                *stream << "  ";
        }
        if (containsEnd && WNEFLAG(NB_SKIPPED_NODES)) {
            // Next siblings will not contain endXP and won't be written: show how many they are
            ldomNode* parent = node->getParentNode();
            int nbIgnoredFollowingSiblings = parent ? (parent->getChildCount() - 1 - node->getNodeIndex()) : 0;
            if (nbIgnoredFollowingSiblings) {
                if (doIndentBeforeEndTag)
                    for (int i = indentBaseLevel; i < level; i++)
                        *stream << "  ";
                *stream << "[…" << lString8().appendDecimal(nbIgnoredFollowingSiblings) << "…]";
                if (doNewLineAfterEndTag)
                    *stream << "\n";
            }
        }
        if (isInitialNode && cssFiles.length() == 0 && WNEFLAG(GET_CSS_FILES) && !node->isRoot()) {
            // We have gathered CSS files as we walked the DOM, which we usually
            // do from the root node if we want CSS files.
            // In case we started from an inner node, and we are requested for
            // CSS files - but we have none - walk the DOM back to gather them.
            ldomNode* pnode = node->getParentNode();
            for (; pnode && !pnode->isNull() && !pnode->isRoot(); pnode = pnode->getParentNode()) {
                if (pnode->getNodeId() == el_DocFragment || pnode->getNodeId() == el_body) {
                    // The CSS file in StyleSheet="" attribute was the first one seen by
                    // crengine, so add it first to cssFiles
                    if (pnode->hasAttribute(attr_StyleSheet)) {
                        lString32 cssFile = pnode->getAttributeValue(attr_StyleSheet);
                        if (!cssFiles.contains(cssFile))
                            cssFiles.add(cssFile);
                    }
                    // And then the CSS files in @import in the <stylesheet> element
                    if (pnode->getChildCount() > 0) {
                        ldomNode* styleNode = pnode->getFirstChild();
                        if (styleNode && styleNode->getNodeId() == el_stylesheet) {
                            // Do as done above
                            lString8 txt = pnode->getText8();
                            const char* s = txt.c_str();
                            while (true) {
                                lString8 import_file;
                                if (!LVProcessStyleSheetImport(s, import_file)) {
                                    break;
                                }
                                lString32 cssFile = LVCombinePaths(pnode->getAttributeValue(attr_href), Utf8ToUnicode(import_file));
                                if (!cssFile.empty() && !cssFiles.contains(cssFile)) {
                                    cssFiles.add(cssFile);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
