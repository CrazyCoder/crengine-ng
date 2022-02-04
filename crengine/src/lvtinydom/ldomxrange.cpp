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

#include <ldomxrange.h>
#include <ldomdocument.h>
#include <lvrend.h>
#include <lvstreamutils.h>

#include "renderrectaccessor.h"
#include "writenodeex.h"
#include "ldomwordscollector.h"
#include "ldomtextcollector.h"

static const ldomXPointerEx& _max(const ldomXPointerEx& v1, const ldomXPointerEx& v2) {
    int c = v1.compare(v2);
    if (c >= 0)
        return v1;
    else
        return v2;
}

static const ldomXPointerEx& _min(const ldomXPointerEx& v1, const ldomXPointerEx& v2) {
    int c = v1.compare(v2);
    if (c <= 0)
        return v1;
    else
        return v2;
}

/// copy constructor of full node range
ldomXRange::ldomXRange(ldomNode* p, bool fitEndToLastInnerChild)
        : _start(p, 0)
        , _end(p, p->isText() ? p->getText().length() : p->getChildCount())
        , _flags(1) {
    // Note: the above initialization seems wrong: for a non-text
    // node, offset seems of no-use, and setting it to the number
    // of children wouldn't matter (and if the original aim was to
    // extend end to include the last child, the range would ignore
    // this last child descendants).
    // The following change might well be the right behaviour expected
    // from ldomXRange(ldomNode) and fixing a bug, but let's keep
    // this "fixed" behaviour an option
    if (fitEndToLastInnerChild && !p->isText()) {
        // Update _end to point to the last deepest inner child node,
        // and to the end of its text if it is a text npde.
        ldomXPointerEx tmp = _start;
        if (tmp.lastInnerNode(true)) {
            _end = tmp;
        }
    }
    // Note: code that walks or compare a ldomXRange may include or
    // exclude the _end: most often, it's excluded.
    // If it is a text node, the end points to text.length(), so after the
    // last char, and it then includes the last char.
    // If it is a non-text node, we could choose to include or exclude it
    // in XPointers comparisons. Including it would have the node included,
    // but not its children (because a child is after its parent in
    // comparisons), which feels strange.
    // So, excluding it looks like the sanest choice.
    // But then, with fitEndToLastInnerChild, if that last inner child
    // is a <IMG> node, it will be the _end, but won't then be included
    // in the range... The proper way to include it then would be to use
    // ldomXPointerEx::nextOuterElement(), but this is just a trick (it
    // would fail if that node is the last in the document, and
    // getNearestParent() would move up unnecessary ancestors...)
    // So, better check the functions that we use to see how they would
    // cope with that case.
}

/// create intersection of two ranges
ldomXRange::ldomXRange(const ldomXRange& v1, const ldomXRange& v2)
        : _start(_max(v1._start, v2._start))
        , _end(_min(v1._end, v2._end)) {
}

/// returns true if this interval intersects specified interval
bool ldomXRange::checkIntersection(ldomXRange& v) {
    if (isNull() || v.isNull())
        return false;
    if (_end.compare(v._start) < 0)
        return false;
    if (_start.compare(v._end) > 0)
        return false;
    return true;
}

static bool findText(const lString32& str, int& pos, int& endpos, const lString32& pattern) {
    int len = pattern.length();
    if (pos < 0 || pos + len > (int)str.length())
        return false;
    const lChar32* s1 = str.c_str() + pos;
    const lChar32* s2 = pattern.c_str();
    int nlen = str.length() - pos - len;
    for (int j = 0; j <= nlen; j++) {
        bool matched = true;
        int nsofthyphens = 0; // There can be soft-hyphen in str, but not in pattern
        for (int i = 0; i < len; i++) {
            while (i + nsofthyphens < nlen && s1[i + nsofthyphens] == UNICODE_SOFT_HYPHEN_CODE) {
                nsofthyphens += 1;
            }
            if (s1[i + nsofthyphens] != s2[i]) {
                matched = false;
                break;
            }
        }
        if (matched) {
            endpos = pos + len + nsofthyphens;
            return true;
        }
        s1++;
        pos++;
    }
    return false;
}

static bool findTextRev(const lString32& str, int& pos, int& endpos, const lString32& pattern) {
    int len = pattern.length();
    if (pos + len > (int)str.length())
        pos = str.length() - len;
    if (pos < 0)
        return false;
    const lChar32* s1 = str.c_str() + pos;
    const lChar32* s2 = pattern.c_str();
    int nlen = pos;
    for (int j = nlen; j >= 0; j--) {
        bool matched = true;
        int nsofthyphens = 0; // There can be soft-hyphen in str, but not in pattern
        for (int i = 0; i < len; i++) {
            while (i + nsofthyphens < nlen && s1[i + nsofthyphens] == UNICODE_SOFT_HYPHEN_CODE) {
                nsofthyphens += 1;
            }
            if (s1[i + nsofthyphens] != s2[i]) {
                matched = false;
                break;
            }
        }
        if (matched) {
            endpos = pos + len + nsofthyphens;
            return true;
        }
        s1--;
        pos--;
    }
    return false;
}

/// searches for specified text inside range
bool ldomXRange::findText(lString32 pattern, bool caseInsensitive, bool reverse, LVArray<ldomWord>& words, int maxCount, int maxHeight, int maxHeightCheckStartY, bool checkMaxFromStart) {
    if (caseInsensitive)
        pattern.lowercase();
    words.clear();
    if (pattern.empty())
        return false;
    if (reverse) {
        // reverse search
        if (!_end.isText()) {
            _end.prevVisibleText();
            lString32 txt = _end.getNode()->getText();
            _end.setOffset(txt.length());
        }
        int firstFoundTextY = -1;
        while (!isNull()) {
            lString32 txt = _end.getNode()->getText();
            int offs = _end.getOffset();
            int endpos;

            if (firstFoundTextY != -1 && maxHeight > 0) {
                ldomXPointer p(_end.getNode(), offs);
                int currentTextY = p.toPoint().y;
                if (currentTextY < firstFoundTextY - maxHeight)
                    return words.length() > 0;
            }

            if (caseInsensitive)
                txt.lowercase();

            while (::findTextRev(txt, offs, endpos, pattern)) {
                if (firstFoundTextY == -1 && maxHeight > 0) {
                    ldomXPointer p(_end.getNode(), offs);
                    int currentTextY = p.toPoint().y;
                    if (maxHeightCheckStartY == -1 || currentTextY <= maxHeightCheckStartY)
                        firstFoundTextY = currentTextY;
                }
                words.add(ldomWord(_end.getNode(), offs, endpos));
                offs--;
            }
            if (!_end.prevVisibleText())
                break;
            txt = _end.getNode()->getText();
            _end.setOffset(txt.length());
            if (words.length() >= maxCount)
                break;
        }
    } else {
        // direct search
        if (!_start.isText())
            _start.nextVisibleText();
        int firstFoundTextY = -1;
        if (checkMaxFromStart) {
            ldomXPointer p(_start.getNode(), _start.getOffset());
            firstFoundTextY = p.toPoint().y;
        }
        while (!isNull()) {
            int offs = _start.getOffset();
            int endpos;

            if (firstFoundTextY != -1 && maxHeight > 0) {
                ldomXPointer p(_start.getNode(), offs);
                int currentTextY = p.toPoint().y;
                if ((checkMaxFromStart && currentTextY >= firstFoundTextY + maxHeight) ||
                    currentTextY > firstFoundTextY + maxHeight)
                    return words.length() > 0;
            }

            lString32 txt = _start.getNode()->getText();
            if (caseInsensitive)
                txt.lowercase();

            while (::findText(txt, offs, endpos, pattern)) {
                if (firstFoundTextY == -1 && maxHeight > 0) {
                    ldomXPointer p(_start.getNode(), offs);
                    int currentTextY = p.toPoint().y;
                    if (checkMaxFromStart) {
                        if (currentTextY >= firstFoundTextY + maxHeight)
                            return words.length() > 0;
                    } else {
                        if (maxHeightCheckStartY == -1 || currentTextY >= maxHeightCheckStartY)
                            firstFoundTextY = currentTextY;
                    }
                }
                words.add(ldomWord(_start.getNode(), offs, endpos));
                offs++;
            }
            if (!_start.nextVisibleText())
                break;
            if (words.length() >= maxCount)
                break;
        }
    }
    return words.length() > 0;
}

/// returns rectangle (in doc coordinates) for range. Returns true if found.
// Note that this works correctly only when start and end are in the
// same text node.
bool ldomXRange::getRectEx(lvRect& rect, bool& isSingleLine) {
    isSingleLine = false;
    if (isNull())
        return false;
    // get start and end rects
    lvRect rc1;
    lvRect rc2;
    // inner=true if enhanced rendering, to directly get the inner coordinates,
    // so no need to compute paddings (as done below for legacy rendering)
    if (!getStart().getRect(rc1, true) || !getEnd().getRect(rc2, true))
        return false;
    ldomNode* finalNode1 = getStart().getFinalNode();
    ldomNode* finalNode2 = getEnd().getFinalNode();
    if (!finalNode1 || !finalNode2) {
        // Shouldn't happen, but prevent a segfault in case some other bug
        // in initNodeRendMethod made some text not having an erm_final ancestor.
        if (!finalNode1)
            printf("CRE WARNING: no final parent for range start %s\n", UnicodeToLocal(getStart().toString()).c_str());
        if (!finalNode2)
            printf("CRE WARNING: no final parent for range end %s\n", UnicodeToLocal(getEnd().toString()).c_str());
        return false;
    }
    RenderRectAccessor fmt1(finalNode1);
    RenderRectAccessor fmt2(finalNode2);
    // In legacy mode, we just got the erm_final coordinates, and we must
    // compute and add left/top border and padding (using rc.width() as
    // the base for % is wrong here, and so is rc.height() for padding top)
    if (!RENDER_RECT_HAS_FLAG(fmt1, INNER_FIELDS_SET)) {
        int padding_left = measureBorder(finalNode1, 3) + lengthToPx(finalNode1, finalNode1->getStyle()->padding[0], fmt1.getWidth());
        int padding_top = measureBorder(finalNode1, 0) + lengthToPx(finalNode1, finalNode1->getStyle()->padding[2], fmt1.getWidth());
        rc1.top += padding_top;
        rc1.left += padding_left;
        rc1.right += padding_left;
        rc1.bottom += padding_top;
    }
    if (!RENDER_RECT_HAS_FLAG(fmt2, INNER_FIELDS_SET)) {
        int padding_left = measureBorder(finalNode2, 3) + lengthToPx(finalNode2, finalNode2->getStyle()->padding[0], fmt2.getWidth());
        int padding_top = measureBorder(finalNode2, 0) + lengthToPx(finalNode2, finalNode2->getStyle()->padding[2], fmt2.getWidth());
        rc2.top += padding_top;
        rc2.left += padding_left;
        rc2.right += padding_left;
        rc2.bottom += padding_top;
    }
    if (rc1.top == rc2.top && rc1.bottom == rc2.bottom) {
        // on same line
        rect.left = rc1.left;
        rect.top = rc1.top;
        rect.right = rc2.right;
        rect.bottom = rc2.bottom;
        isSingleLine = true;
        return !rect.isEmpty();
    }
    // on different lines
    ldomNode* parent = getNearestCommonParent();
    if (!parent)
        return false;
    parent->getAbsRect(rect);
    rect.top = rc1.top;
    rect.bottom = rc2.bottom;
    return !rect.isEmpty();
}

// Returns the multiple segments (rectangle for each text line) that
// this ldomXRange spans on the page.
// The text content from S to E on this page will push 4 segments:
//   ......
//   ...S==
//   ======
//   ======
//   ==E..
//   ......
void ldomXRange::getSegmentRects(LVArray<lvRect>& rects) {
    bool go_on = true;
    lvRect lineStartRect = lvRect();
    lvRect nodeStartRect = lvRect();
    lvRect curCharRect = lvRect();
    lvRect prevCharRect = lvRect();
    ldomNode* prevFinalNode = NULL; // to add rect when we cross final nodes

    // We process range text node by text node (I thought rects' y-coordinates
    // comparisons were valid only for a same text node, but it seems all
    // text on a line get the same .top and .bottom, even if they have a
    // smaller font size - but using ldomXRange.getRectEx() on multiple
    // text nodes gives wrong rects for the last chars on a line...)

    // Note: someRect.extend(someOtherRect) and !someRect.isEmpty() expect
    // a rect to have both width and height non-zero. So, make sure
    // in getRectEx() that we always get a rect of width at least 1px,
    // otherwise some lines may not be highlighted.

    // Note: the range end offset is NOT part of the range (it points to the
    // char after, or last char + 1 if it includes the whole text node text)
    ldomXPointerEx rangeEnd = getEnd();
    ldomXPointerEx curPos = ldomXPointerEx(getStart()); // copy, will change
    if (!curPos.isText())                               // we only deal with text nodes: get the first
        go_on = curPos.nextText();

    while (go_on) { // new line or new/continued text node
        // We may have (empty or not if not yet pushed) from previous iteration:
        // lineStartRect : char rect for first char of line, even if from another text node
        // nodeStartRect : char rect of current char at curPos (calculated but not included
        //   in previous line), that is now the start of the line
        // The curPos.getRectEx(charRect) we use returns a rect for a single char, with
        // the width of the char. We then "extend" it to the char at end of line (or end
        // of range) to make a segment that we add to the provided &rects.
        // We use getRectEx() with adjusted=true, for fine tuned glyph rectangles
        // that include the excessive left or right side bearing.

        if (!curPos || curPos.isNull() || curPos.compare(rangeEnd) >= 0) {
            // no more text node, or after end of range: we're done
            break;
        }

        ldomNode* curFinalNode = curPos.getFinalNode();
        if (curFinalNode != prevFinalNode) {
            // Force a new segment if we're crossing final nodes, that is, when
            // we're no more in the same inline context (so we get a new segment
            // for each table cells that may happen to be rendered on the same line)
            if (!lineStartRect.isEmpty()) {
                rects.add(lineStartRect);
                lineStartRect = lvRect(); // reset
            }
            prevFinalNode = curFinalNode;
        }

        int startOffset = curPos.getOffset();
        lString32 nodeText = curPos.getText();
        int textLen = nodeText.length();

        if (startOffset == 0) {       // new text node
            nodeStartRect = lvRect(); // reset
            if (textLen == 0) {       // empty text node (not sure that can happen)
                go_on = curPos.nextText();
                continue;
            }
        }
        // Skip space at start of node or at start of new line
        // (the XML parser made sure we always have a single space
        // at boundaries)
        if (nodeText[startOffset] == ' ') {
            startOffset += 1;
            nodeStartRect = lvRect(); // reset
        }
        if (startOffset >= textLen) { // no more text in this node (or single space node)
            go_on = curPos.nextText();
            nodeStartRect = lvRect(); // reset
            continue;
        }
        curPos.setOffset(startOffset);
        if (nodeStartRect.isEmpty()) { // otherwise, we re-use the one left from previous loop
            // getRectEx() seems to fail on a single no-break-space, but we
            // are not supposed to see a no-br space at start of line.
            // Anyway, try next chars if first one(s) fails
            while (startOffset <= textLen - 2 && !curPos.getRectEx(nodeStartRect, true)) {
                // printf("#### curPos.getRectEx(nodeStartRect:%d) failed\n", startOffset);
                startOffset++;
                curPos.setOffset(startOffset);
                nodeStartRect = lvRect(); // reset
            }
            // last try with the last char (startOffset = textLen-1):
            if (!curPos.getRectEx(nodeStartRect, true)) {
                // printf("#### curPos.getRectEx(nodeStartRect) failed\n");
                // getRectEx() returns false when a node is invisible, so we just
                // go processing next text node on failure (it may fail for other
                // reasons that we won't notice, except for may be holes in the
                // highlighting)
                go_on = curPos.nextText(); // skip this text node
                nodeStartRect = lvRect();  // reset
                continue;
            }
        }
        if (lineStartRect.isEmpty()) {
            lineStartRect = nodeStartRect; // re-use the one already computed
        }
        // This would help noticing a line-feed-back-to-start-of-line:
        //   else if (nodeStartRect.left < lineStartRect.right)
        // but it makes a 2-lines-tall single segment if text-indent is larger
        // than previous line end.
        // So, use .top comparison
        else if (nodeStartRect.top > lineStartRect.top) {
            // We ended last node on a line, but a new node starts (or previous
            // one continues) on a different line.
            // And we have a not-yet-added lineStartRect: add it as it is
            rects.add(lineStartRect);
            lineStartRect = nodeStartRect; // start line on current node
        }

        // 1) Look if text node contains end of range (probably the case
        // when only a few words are highlighted)
        if (curPos.getNode() == rangeEnd.getNode() && rangeEnd.getOffset() <= textLen) {
            curCharRect = lvRect();
            curPos.setOffset(rangeEnd.getOffset() - 1); // Range end is not part of the range
            if (!curPos.getRectEx(curCharRect, true)) {
                // printf("#### curPos.getRectEx(textLen=%d) failed\n", textLen);
                go_on = curPos.nextText(); // skip this text node
                nodeStartRect = lvRect();  // reset
                continue;
            }
            if (curCharRect.top == nodeStartRect.top) { // end of range is on current line
                // (Two offsets in a same text node with the same tops are on the same line)
                lineStartRect.extend(curCharRect);
                // lineStartRect will be added after loop exit
                break; // we're done
            }
        }

        // 2) Look if the full text node is contained on the line
        // Ignore (possibly collapsed) space at end of text node
        curPos.setOffset(nodeText[textLen - 1] == ' ' ? textLen - 2 : textLen - 1);
        curCharRect = lvRect();
        if (!curPos.getRectEx(curCharRect, true)) {
            // printf("#### curPos.getRectEx(textLen=%d) failed\n", textLen);
            go_on = curPos.nextText(); // skip this text node
            nodeStartRect = lvRect();  // reset
            continue;
        }
        if (curCharRect.top == nodeStartRect.top) {
            // Extend line up to the end of this node, but don't add it yet,
            // lineStartRect can still be extended with (parts of) next text nodes
            lineStartRect.extend(curCharRect);
            nodeStartRect = lvRect();  // reset
            go_on = curPos.nextText(); // go processing next text node
            continue;
        }

        // 3) Current text node's end is not on our line:
        // scan it char by char to see where it changes line
        // (we could use binary search to reduce the number of iterations)
        curPos.setOffset(startOffset);
        prevCharRect = nodeStartRect;
        for (int i = startOffset + 1; i <= textLen - 1; i++) {
            // skip spaces (but let soft-hyphens in, so they are part of the
            // highlight when they are shown at end of line)
            lChar32 c = nodeText[i];
            if (c == ' ') // || c == 0x00AD)
                continue;
            curPos.setOffset(i);
            curCharRect = lvRect(); // reset
            if (!curPos.getRectEx(curCharRect, true)) {
                // printf("#### curPos.getRectEx(char=%d) failed\n", i);
                // Can happen with non-break-space and may be others,
                // just try with next char
                continue;
            }
            if (curPos.compare(rangeEnd) >= 0) {
                // should not happen, we should have dealt with it as 1)
                // printf("??????????? curPos.getRectEx(char=%d) end of range\n", i);
                go_on = false;        // don't break yet, need to add what we met before
                curCharRect.top = -1; // force adding prevCharRect
            }
            if (curCharRect.top != nodeStartRect.top) { // no more on the same line
                if (!prevCharRect.isEmpty()) {          // (should never be empty)
                    // We got previously a rect on this line: it's the end of line
                    lineStartRect.extend(prevCharRect);
                    rects.add(lineStartRect);
                }
                // Continue with this text node, but on a new line
                nodeStartRect = curCharRect;
                lineStartRect = lvRect(); // reset
                break;                    // break for (i<textLen) loop
            }
            prevCharRect = curCharRect; // still on the line: candidate for end of line
            if (!go_on)
                break; // we're done
        }
    }
    // Add any lineStartRect not yet added
    if (!lineStartRect.isEmpty()) {
        rects.add(lineStartRect);
    }
}

/// sets range to nearest word bounds, returns true if success
bool ldomXRange::getWordRange(ldomXRange& range, ldomXPointer& p) {
    ldomNode* node = p.getNode();
    if (!node->isText())
        return false;
    int pos = p.getOffset();
    lString32 txt = node->getText();
    if (pos < 0)
        pos = 0;
    if (pos > (int)txt.length())
        pos = txt.length();
    int endpos = pos;
    for (;;) {
        lChar32 ch = txt[endpos];
        if (ch == 0 || ch == ' ')
            break;
        endpos++;
    }
    /*
    // include trailing space
    for (;;) {
        lChar32 ch = txt[endpos];
        if ( ch==0 || ch!=' ' )
            break;
        endpos++;
    }
    */
    for (;;) {
        if (pos == 0)
            break;
        if (txt[pos] != ' ')
            break;
        pos--;
    }
    for (;;) {
        if (pos == 0)
            break;
        if (txt[pos - 1] == ' ')
            break;
        pos--;
    }
    ldomXRange r(ldomXPointer(node, pos), ldomXPointer(node, endpos));
    range = r;
    return true;
}

/// returns nearest common element for start and end points
ldomNode* ldomXRange::getNearestCommonParent() {
    ldomXPointerEx start(getStart());
    ldomXPointerEx end(getEnd());
    while (start.getLevel() > end.getLevel() && start.parent())
        ;
    while (start.getLevel() < end.getLevel() && end.parent())
        ;
    /*
    while ( start.getIndex()!=end.getIndex() && start.parent() && end.parent() )
        ;
    if ( start.getNode()==end.getNode() )
        return start.getNode();
    return NULL;
    */
    // This above seems wrong: we could have start and end on the same level,
    // but in different parent nodes, with still the same index among these
    // different parent nodes' children.
    // Best to check for node identity, till we find the same parent,
    // or the root node
    while (start.getNode() != end.getNode() && start.parent() && end.parent())
        ;
    return start.getNode();
}

/// returns HTML (serialized from the DOM, may be different from the source HTML)
/// puts the paths of the linked css files met into the provided lString32Collection cssFiles
lString8 ldomXRange::getHtml(lString32Collection& cssFiles, int wflags, bool fromRootNode) {
    if (isNull())
        return lString8::empty_str;
    sort();
    ldomNode* startNode;
    if (fromRootNode) {
        startNode = getStart().getNode()->getDocument()->getRootNode();
        if (startNode->getChildCount() == 1) // start HTML with first child (<body>)
            startNode = startNode->getFirstChild();
    } else {
        // We need to start from the nearest common parent, to get balanced HTML
        startNode = getNearestCommonParent();
    }
    LVStreamRef stream = LVCreateMemoryStream(NULL, 0, false, LVOM_WRITE);
    writeNodeEx(stream.get(), startNode, cssFiles, wflags, getStart(), getEnd());
    int size = stream->GetSize();
    LVArray<char> buf(size + 1, '\0');
    stream->Seek(0, LVSEEK_SET, NULL);
    stream->Read(buf.get(), size, NULL);
    buf[size] = 0;
    lString8 html = lString8(buf.get());
    return html;
}

/// if start is after end, swap start and end
void ldomXRange::sort() {
    if (_start.isNull() || _end.isNull())
        return;
    if (_start.compare(_end) > 0) {
        ldomXPointer p1(_start);
        ldomXPointer p2(_end);
        _start = p2;
        _end = p1;
    }
}

/// run callback for each node in range
void ldomXRange::forEach(ldomNodeCallback* callback) {
    if (isNull())
        return;
    ldomXRange pos(_start, _end, 0);
    bool allowGoRecurse = true;
    while (!pos._start.isNull() && pos._start.compare(_end) < 0) {
        // do something
        ldomNode* node = pos._start.getNode();
        //lString32 path = pos._start.toString();
        //CRLog::trace( "%s", UnicodeToUtf8(path).c_str() );
        if (node->isElement()) {
            allowGoRecurse = callback->onElement(&pos.getStart());
        } else if (node->isText()) {
            lString32 txt = node->getText();
            pos._end = pos._start;
            pos._start.setOffset(0);
            pos._end.setOffset(txt.length());
            if (_start.getNode() == node) {
                pos._start.setOffset(_start.getOffset());
            }
            if (_end.getNode() == node && pos._end.getOffset() > _end.getOffset()) {
                pos._end.setOffset(_end.getOffset());
            }
            callback->onText(&pos);
            allowGoRecurse = false;
        }
        // move to next item
        bool stop = false;
        if (!allowGoRecurse || !pos._start.child(0)) {
            while (!pos._start.nextSibling()) {
                if (!pos._start.parent()) {
                    stop = true;
                    break;
                }
            }
        }
        if (stop)
            break;
    }
}

/// get all words from specified range
void ldomXRange::getRangeWords(LVArray<ldomWord>& list) {
    ldomWordsCollector collector(list);
    forEach(&collector);
}

/// returns text between two XPointer positions
lString32 ldomXRange::getRangeText(lChar32 blockDelimiter, int maxTextLen) {
    ldomTextCollector callback(blockDelimiter, maxTextLen);
    forEach(&callback);
    return removeSoftHyphens(callback.getText());
}

/// returns href attribute of <A> element, plus xpointer of <A> element itself
lString32 ldomXRange::getHRef(ldomXPointer& a_xpointer) {
    if (isNull())
        return lString32::empty_str;
    return _start.getHRef(a_xpointer);
}

/// returns href attribute of <A> element, null string if not found
lString32 ldomXRange::getHRef() {
    if (isNull())
        return lString32::empty_str;
    return _start.getHRef();
}
