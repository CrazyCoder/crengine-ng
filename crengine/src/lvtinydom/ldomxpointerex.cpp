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

#include <ldomxpointerex.h>
#include <ldomdocument.h>

// TODO: implement better behavior
inline bool IsUnicodeSpace(lChar32 ch) {
    //return ch==' ';
    switch ((unsigned short)ch) {
        case 0x0020: // SPACE
        case 0x00A0: // NO-BREAK SPACE
        case 0x2000: // EN QUAD
        case 0x2001: // EM QUAD
        case 0x2002: // EN SPACE
        case 0x2003: // EM SPACE
        case 0x2004: // THREE-PER-EM SPACE
        case 0x2005: // FOUR-PER-EM SPACE
        case 0x202F: // NARROW NO-BREAK SPACE
        case 0x3000: // IDEOGRAPHIC SPACE
            return true;
    }
    return false;
}

// TODO: implement better behavior
inline bool IsUnicodeSpaceOrNull(lChar32 ch) {
    return ch == 0 || IsUnicodeSpace(ch);
}

// Note:
//  ALL calls to IsUnicodeSpace and IsUnicodeSpaceOrNull in
//  the *VisibleWord* functions below have been replaced with
//  calls to IsWordSeparator and IsWordSeparatorOrNull.
//  The *Sentence* functions have not beed modified, and have not been
//  tested against this change to the *VisibleWord* functions that
//  they use (but KOReader does not use these *Sentence* functions).

// For better accuracy than IsUnicodeSpace for detecting words
inline bool IsWordSeparator(lChar32 ch) {
    return lStr_isWordSeparator(ch);
}

inline bool IsWordSeparatorOrNull(lChar32 ch) {
    if (ch == 0)
        return true;
    return IsWordSeparator(ch);
}

inline bool canWrapWordBefore(lChar32 ch) {
    return ch >= 0x2e80 && ch < 0x2CEAF;
}

inline bool canWrapWordAfter(lChar32 ch) {
    return ch >= 0x2e80 && ch < 0x2CEAF;
}

void ldomXPointerEx::initIndex() {
    int m[MAX_DOM_LEVEL];
    ldomNode* p = getNode();
    _level = 0;
    while (p) {
        m[_level] = p->getNodeIndex();
        _level++;
        if (_level == MAX_DOM_LEVEL) {
            getDocument()->printWarning("ldomXPointerEx level overflow (too many nested nodes)", 1);
            break;
        }
        p = p->getParentNode();
    }
    for (int i = 0; i < _level; i++) {
        _indexes[i] = m[_level - i - 1];
    }
}

/// move to sibling #
bool ldomXPointerEx::sibling(int index) {
    if (_level <= 1)
        return false;
    ldomNode* p = getNode()->getParentNode();
    if (!p || index < 0 || index >= (int)p->getChildCount())
        return false;
    setNode(p->getChildNode(index));
    setOffset(0);
    _indexes[_level - 1] = index;
    return true;
}

/// move to next sibling
bool ldomXPointerEx::nextSibling() {
    if (_level <= 1)
        return false;
    return sibling(_indexes[_level - 1] + 1);
}

/// move to previous sibling
bool ldomXPointerEx::prevSibling() {
    if (_level <= 1)
        return false;
    return sibling(_indexes[_level - 1] - 1);
}

/// move to next sibling element
bool ldomXPointerEx::nextSiblingElement() {
    if (_level <= 1)
        return false;
    ldomNode* node = getNode();
    ldomNode* p = node->getParentNode();
    for (int i = _indexes[_level - 1] + 1; i < (int)p->getChildCount(); i++) {
        if (p->getChildNode(i)->isElement())
            return sibling(i);
    }
    return false;
}

/// move to previous sibling element
bool ldomXPointerEx::prevSiblingElement() {
    if (_level <= 1)
        return false;
    ldomNode* node = getNode();
    ldomNode* p = node->getParentNode();
    for (int i = _indexes[_level - 1] - 1; i >= 0; i--) {
        if (p->getChildNode(i)->isElement())
            return sibling(i);
    }
    return false;
}

/// move to next sibling or parent's next sibling
bool ldomXPointerEx::nextOuterElement() {
    if (!ensureElement())
        return false;
    for (;;) {
        if (nextSiblingElement())
            return true;
        if (!parent())
            return false;
    }
}

/// move to (end of) last and deepest child node descendant of current node
bool ldomXPointerEx::lastInnerNode(bool toTextEnd) {
    if (!getNode())
        return false;
    while (lastChild()) {
    }
    if (isText() && toTextEnd) {
        setOffset(getNode()->getText().length());
    }
    return true;
}

/// move to (end of) last and deepest child text node descendant of current node
bool ldomXPointerEx::lastInnerTextNode(bool toTextEnd) {
    if (!getNode())
        return false;
    if (isText()) {
        if (toTextEnd)
            setOffset(getNode()->getText().length());
        return true;
    }
    if (lastChild()) {
        do {
            if (lastInnerTextNode(toTextEnd))
                return true;
        } while (prevSibling());
        parent();
    }
    return false;
}

/// move to parent
bool ldomXPointerEx::parent() {
    if (_level <= 1)
        return false;
    setNode(getNode()->getParentNode());
    setOffset(0);
    _level--;
    return true;
}

/// move to child #
bool ldomXPointerEx::child(int index) {
    if (_level >= MAX_DOM_LEVEL)
        return false;
    int count = getNode()->getChildCount();
    if (index < 0 || index >= count)
        return false;
    _indexes[_level++] = index;
    setNode(getNode()->getChildNode(index));
    setOffset(0);
    return true;
}

/// compare two pointers, returns -1, 0, +1
int ldomXPointerEx::compare(const ldomXPointerEx& v) const {
    int i;
    for (i = 0; i < _level && i < v._level; i++) {
        if (_indexes[i] < v._indexes[i])
            return -1;
        if (_indexes[i] > v._indexes[i])
            return 1;
    }
    if (_level < v._level) {
        return -1;
        //        if ( getOffset() < v._indexes[i] )
        //            return -1;
        //        if ( getOffset() > v._indexes[i] )
        //            return 1;
        //        return -1;
    }
    if (_level > v._level) {
        if (_indexes[i] < v.getOffset())
            return -1;
        if (_indexes[i] > v.getOffset())
            return 1;
        return 1;
    }
    if (getOffset() < v.getOffset())
        return -1;
    if (getOffset() > v.getOffset())
        return 1;
    return 0;
}

/// calls specified function recursively for all elements of DOM tree
void ldomXPointerEx::recurseElements(void (*pFun)(ldomXPointerEx& node)) {
    if (!isElement())
        return;
    pFun(*this);
    if (child(0)) {
        do {
            recurseElements(pFun);
        } while (nextSibling());
        parent();
    }
}

/// calls specified function recursively for all nodes of DOM tree
void ldomXPointerEx::recurseNodes(void (*pFun)(ldomXPointerEx& node)) {
    if (!isElement())
        return;
    pFun(*this);
    if (child(0)) {
        do {
            recurseElements(pFun);
        } while (nextSibling());
        parent();
    }
}

/// searches path for element with specific id, returns level at which element is founs, 0 if not found
int ldomXPointerEx::findElementInPath(lUInt16 id) {
    if (!ensureElement())
        return 0;
    ldomNode* e = getNode();
    for (; e != NULL; e = e->getParentNode()) {
        if (e->getNodeId() == id) {
            return e->getNodeLevel();
        }
    }
    return 0;
}

bool ldomXPointerEx::ensureFinal() {
    if (!ensureElement())
        return false;
    int cnt = 0;
    int foundCnt = -1;
    ldomNode* e = getNode();
    for (; e != NULL; e = e->getParentNode()) {
        if (e->getRendMethod() == erm_final) {
            foundCnt = cnt;
        }
        cnt++;
    }
    if (foundCnt < 0)
        return false;
    for (int i = 0; i < foundCnt; i++)
        parent();
    // curent node is final formatted element (e.g. paragraph)
    return true;
}

/// ensure that current node is element (move to parent, if not - from text node to element)
bool ldomXPointerEx::ensureElement() {
    ldomNode* node = getNode();
    if (!node)
        return false;
    if (node->isText()) {
        if (!parent())
            return false;
        node = getNode();
    }
    if (!node || !node->isElement())
        return false;
    return true;
}

/// move to first child of current node
bool ldomXPointerEx::firstChild() {
    return child(0);
}

/// move to last child of current node
bool ldomXPointerEx::lastChild() {
    int count = getNode()->getChildCount();
    if (count <= 0)
        return false;
    return child(count - 1);
}

/// move to first element child of current node
bool ldomXPointerEx::firstElementChild() {
    ldomNode* node = getNode();
    int count = node->getChildCount();
    for (int i = 0; i < count; i++) {
        if (node->getChildNode(i)->isElement())
            return child(i);
    }
    return false;
}

/// move to last element child of current node
bool ldomXPointerEx::lastElementChild() {
    ldomNode* node = getNode();
    int count = node->getChildCount();
    for (int i = count - 1; i >= 0; i--) {
        if (node->getChildNode(i)->isElement())
            return child(i);
    }
    return false;
}

/// forward iteration by elements of DOM three
bool ldomXPointerEx::nextElement() {
    if (!ensureElement())
        return false;
    if (firstElementChild())
        return true;
    for (;;) {
        if (nextSiblingElement())
            return true;
        if (!parent())
            return false;
    }
}

/// returns true if current node is visible element with render method == erm_final
bool ldomXPointerEx::isVisibleFinal() {
    if (!isElement())
        return false;
    int cnt = 0;
    int foundCnt = -1;
    ldomNode* e = getNode();
    for (; e != NULL; e = e->getParentNode()) {
        switch (e->getRendMethod()) {
            case erm_final:
                foundCnt = cnt;
                break;
            case erm_invisible:
                foundCnt = -1;
                break;
            default:
                break;
        }
        cnt++;
    }
    if (foundCnt != 0)
        return false;
    // curent node is visible final formatted element (e.g. paragraph)
    return true;
}

/// move to next visible text node
bool ldomXPointerEx::nextVisibleText(bool thisBlockOnly) {
    ldomXPointerEx backup;
    if (thisBlockOnly)
        backup = *this;
    while (nextText(thisBlockOnly)) {
        if (isVisible())
            return true;
    }
    if (thisBlockOnly)
        *this = backup;
    return false;
}

/// returns true if current node is visible element or text
bool ldomXPointerEx::isVisible() {
    ldomNode* p;
    ldomNode* node = getNode();
    if (node && node->isText())
        p = node->getParentNode();
    else
        p = node;
    while (p) {
        if (p->getRendMethod() == erm_invisible)
            return false;
        /* This would feel needed now that we added support for visibility:hidden.
         * But it may have side effects that I don't want to investigate.
         * Let's say that hidden (not visible) does not mean it should not be
         * audible, searchable and selectable; if it's hidden, we should be
         * allowed to find it :)
        if ( p->getStyle()->visibility >= css_v_hidden )
            return false;
        */
        p = p->getParentNode();
    }
    return true;
}

/// move to next text node
bool ldomXPointerEx::nextText(bool thisBlockOnly) {
    ldomNode* block = NULL;
    if (thisBlockOnly)
        block = getThisBlockNode();
    setOffset(0);
    while (firstChild()) {
        if (isText())
            return (!thisBlockOnly || getThisBlockNode() == block);
    }
    for (;;) {
        while (nextSibling()) {
            if (isText())
                return (!thisBlockOnly || getThisBlockNode() == block);
            while (firstChild()) {
                if (isText())
                    return (!thisBlockOnly || getThisBlockNode() == block);
            }
        }
        if (!parent())
            return false;
    }
}

/// move to previous text node
bool ldomXPointerEx::prevText(bool thisBlockOnly) {
    ldomNode* block = NULL;
    if (thisBlockOnly)
        block = getThisBlockNode();
    setOffset(0);
    for (;;) {
        while (prevSibling()) {
            if (isText())
                return (!thisBlockOnly || getThisBlockNode() == block);
            while (lastChild()) {
                if (isText())
                    return (!thisBlockOnly || getThisBlockNode() == block);
            }
        }
        if (!parent())
            return false;
    }
}

/// move to previous visible text node
bool ldomXPointerEx::prevVisibleText(bool thisBlockOnly) {
    ldomXPointerEx backup;
    if (thisBlockOnly)
        backup = *this;
    while (prevText(thisBlockOnly))
        if (isVisible())
            return true;
    if (thisBlockOnly)
        *this = backup;
    return false;
}

/// move to previous visible char
bool ldomXPointerEx::prevVisibleChar(bool thisBlockOnly) {
    if (isNull())
        return false;
    if (!isText() || !isVisible() || _data->getOffset() == 0) {
        // move to previous text
        if (!prevVisibleText(thisBlockOnly))
            return false;
        ldomNode* node = getNode();
        lString32 text = node->getText();
        int textLen = text.length();
        _data->setOffset(textLen);
    }
    _data->addOffset(-1);
    return true;
}

/// move to next visible char
bool ldomXPointerEx::nextVisibleChar(bool thisBlockOnly) {
    if (isNull())
        return false;
    if (!isText() || !isVisible()) {
        // move to next text
        if (!nextVisibleText(thisBlockOnly))
            return false;
        _data->setOffset(0);
        return true;
    }
    ldomNode* node = getNode();
    lString32 text = node->getText();
    int textLen = text.length();
    if (_data->getOffset() == textLen) {
        // move to next text
        if (!nextVisibleText(thisBlockOnly))
            return false;
        _data->setOffset(0);
        return true;
    }
    _data->addOffset(1);
    return true;
}

bool ldomXPointerEx::isVisibleWordChar() {
    if (isNull())
        return false;
    if (!isText() || !isVisible())
        return false;
    ldomNode* node = getNode();
    lString32 text = node->getText();
    return !IsWordSeparator(text[_data->getOffset()]);
}

/// move to previous visible word beginning
bool ldomXPointerEx::prevVisibleWordStart(bool thisBlockOnly) {
    if (isNull())
        return false;
    ldomNode* node = NULL;
    lString32 text;
    for (;;) {
        if (!isText() || !isVisible() || _data->getOffset() == 0) {
            // move to previous text
            if (!prevVisibleText(thisBlockOnly))
                return false;
            node = getNode();
            text = node->getText();
            int textLen = text.length();
            _data->setOffset(textLen);
        } else {
            node = getNode();
            text = node->getText();
        }
        bool foundNonSeparator = false;
        while (_data->getOffset() > 0 && IsWordSeparator(text[_data->getOffset() - 1]))
            _data->addOffset(-1); // skip preceeding space if any (we were on a visible word start)
        while (_data->getOffset() > 0) {
            if (IsWordSeparator(text[_data->getOffset() - 1]))
                break;
            foundNonSeparator = true;
            _data->addOffset(-1);
            if (canWrapWordBefore(text[_data->getOffset()])) // CJK char
                break;
        }
        if (foundNonSeparator)
            return true;
    }
}

/// move to previous visible word end
bool ldomXPointerEx::prevVisibleWordEnd(bool thisBlockOnly) {
    if (isNull())
        return false;
    ldomNode* node = NULL;
    lString32 text;
    bool moved = false;
    for (;;) {
        if (!isText() || !isVisible() || _data->getOffset() == 0) {
            // move to previous text
            if (!prevVisibleText(thisBlockOnly))
                return false;
            node = getNode();
            text = node->getText();
            int textLen = text.length();
            _data->setOffset(textLen);
            moved = true;
        } else {
            node = getNode();
            text = node->getText();
        }
        // skip separators
        while (_data->getOffset() > 0 && IsWordSeparator(text[_data->getOffset() - 1])) {
            _data->addOffset(-1);
            moved = true;
        }
        if (moved && _data->getOffset() > 0)
            return true; // found!
        // skip non-separators
        while (_data->getOffset() > 0) {
            if (IsWordSeparator(text[_data->getOffset() - 1]))
                break;
            if (moved && canWrapWordAfter(text[_data->getOffset()])) // We moved to a CJK char
                return true;
            moved = true;
            _data->addOffset(-1);
        }
        // skip separators
        while (_data->getOffset() > 0 && IsWordSeparator(text[_data->getOffset() - 1])) {
            _data->addOffset(-1);
            moved = true;
        }
        if (moved && _data->getOffset() > 0)
            return true; // found!
    }
}

/// move to next visible word beginning
bool ldomXPointerEx::nextVisibleWordStart(bool thisBlockOnly) {
    if (isNull())
        return false;
    ldomNode* node = NULL;
    lString32 text;
    int textLen = 0;
    bool moved = false;
    for (;;) {
        if (!isText() || !isVisible()) {
            // move to previous text
            if (!nextVisibleText(thisBlockOnly))
                return false;
            node = getNode();
            text = node->getText();
            textLen = text.length();
            _data->setOffset(0);
            moved = true;
        } else {
            for (;;) {
                node = getNode();
                text = node->getText();
                textLen = text.length();
                if (_data->getOffset() < textLen)
                    break;
                if (!nextVisibleText(thisBlockOnly))
                    return false;
                _data->setOffset(0);
                moved = true;
            }
        }
        // skip separators
        while (_data->getOffset() < textLen && IsWordSeparator(text[_data->getOffset()])) {
            _data->addOffset(1);
            moved = true;
        }
        if (moved && _data->getOffset() < textLen)
            return true;
        // skip non-separators
        while (_data->getOffset() < textLen) {
            if (IsWordSeparator(text[_data->getOffset()]))
                break;
            if (moved && canWrapWordBefore(text[_data->getOffset()])) // We moved to a CJK char
                return true;
            moved = true;
            _data->addOffset(1);
        }
        // skip separators
        while (_data->getOffset() < textLen && IsWordSeparator(text[_data->getOffset()])) {
            _data->addOffset(1);
            moved = true;
        }
        if (moved && _data->getOffset() < textLen)
            return true;
    }
}

/// move to end of current word
bool ldomXPointerEx::thisVisibleWordEnd(bool thisBlockOnly) {
    CR_UNUSED(thisBlockOnly);
    if (isNull())
        return false;
    ldomNode* node = NULL;
    lString32 text;
    int textLen = 0;
    bool moved = false;
    if (!isText() || !isVisible())
        return false;
    node = getNode();
    text = node->getText();
    textLen = text.length();
    if (_data->getOffset() >= textLen)
        return false;
    // skip separators
    while (_data->getOffset() < textLen && IsWordSeparator(text[_data->getOffset()])) {
        _data->addOffset(1);
        //moved = true;
    }
    // skip non-separators
    while (_data->getOffset() < textLen) {
        if (IsWordSeparator(text[_data->getOffset()]))
            break;
        moved = true;
        _data->addOffset(1);
    }
    return moved;
}

/// move to next visible word end
bool ldomXPointerEx::nextVisibleWordEnd(bool thisBlockOnly) {
    if (isNull())
        return false;
    ldomNode* node = NULL;
    lString32 text;
    int textLen = 0;
    //bool moved = false;
    for (;;) {
        if (!isText() || !isVisible()) {
            // move to previous text
            if (!nextVisibleText(thisBlockOnly))
                return false;
            node = getNode();
            text = node->getText();
            textLen = text.length();
            _data->setOffset(0);
            //moved = true;
        } else {
            for (;;) {
                node = getNode();
                text = node->getText();
                textLen = text.length();
                if (_data->getOffset() < textLen)
                    break;
                if (!nextVisibleText(thisBlockOnly))
                    return false;
                _data->setOffset(0);
            }
        }
        bool nonSeparatorFound = false;
        // skip non-separators
        while (_data->getOffset() < textLen) {
            if (IsWordSeparator(text[_data->getOffset()]))
                break;
            nonSeparatorFound = true;
            _data->addOffset(1);
            if (canWrapWordAfter(text[_data->getOffset()])) // We moved to a CJK char
                return true;
        }
        if (nonSeparatorFound)
            return true;
        // skip separators
        while (_data->getOffset() < textLen && IsWordSeparator(text[_data->getOffset()])) {
            _data->addOffset(1);
            //moved = true;
        }
        // skip non-separators
        while (_data->getOffset() < textLen) {
            if (IsWordSeparator(text[_data->getOffset()]))
                break;
            nonSeparatorFound = true;
            _data->addOffset(1);
            if (canWrapWordAfter(text[_data->getOffset()])) // We moved to a CJK char
                return true;
        }
        if (nonSeparatorFound)
            return true;
    }
}

/// move to previous visible word beginning (in sentence)
bool ldomXPointerEx::prevVisibleWordStartInSentence(bool thisBlockOnly) {
    if (isNull())
        return false;
    ldomNode* node = NULL;
    lString32 text;
    for (;;) {
        if (!isText() || !isVisible() || _data->getOffset() == 0) {
            // move to previous text
            if (!prevVisibleText(thisBlockOnly))
                return false;
            node = getNode();
            text = node->getText();
            int textLen = text.length();
            _data->setOffset(textLen);
        } else {
            node = getNode();
            text = node->getText();
        }
        bool foundNonSpace = false;
        while (_data->getOffset() > 0 && IsUnicodeSpace(text[_data->getOffset() - 1]))
            _data->addOffset(-1); // skip preceeding space if any (we were on a visible word start)
        while (_data->getOffset() > 0) {
            if (IsUnicodeSpace(text[_data->getOffset() - 1]))
                break;
            foundNonSpace = true;
            _data->addOffset(-1);
            if (canWrapWordBefore(text[_data->getOffset()])) // CJK char
                break;
        }
        if (foundNonSpace)
            return true;
    }
}

/// move to next visible word beginning (in sentence)
bool ldomXPointerEx::nextVisibleWordStartInSentence(bool thisBlockOnly) {
    if (isNull())
        return false;
    ldomNode* node = NULL;
    lString32 text;
    int textLen = 0;
    bool moved = false;
    for (;;) {
        if (!isText() || !isVisible()) {
            // move to next text
            if (!nextVisibleText(thisBlockOnly))
                return false;
            node = getNode();
            text = node->getText();
            textLen = text.length();
            _data->setOffset(0);
            moved = true;
        } else {
            for (;;) {
                node = getNode();
                text = node->getText();
                textLen = text.length();
                if (_data->getOffset() < textLen)
                    break;
                if (!nextVisibleText(thisBlockOnly))
                    return false;
                _data->setOffset(0);
                moved = true;
            }
        }
        // skip spaces
        while (_data->getOffset() < textLen && IsUnicodeSpace(text[_data->getOffset()])) {
            _data->addOffset(1);
            moved = true;
        }
        if (moved && _data->getOffset() < textLen)
            return true;
        // skip non-spaces
        while (_data->getOffset() < textLen) {
            if (IsUnicodeSpace(text[_data->getOffset()]))
                break;
            if (moved && canWrapWordBefore(text[_data->getOffset()])) // We moved to a CJK char
                return true;
            moved = true;
            _data->addOffset(1);
        }
        // skip spaces
        while (_data->getOffset() < textLen && IsUnicodeSpace(text[_data->getOffset()])) {
            _data->addOffset(1);
            moved = true;
        }
        if (moved && _data->getOffset() < textLen)
            return true;
    }
}

/// move to end of current word
bool ldomXPointerEx::thisVisibleWordEndInSentence() {
    if (isNull())
        return false;
    ldomNode* node = NULL;
    lString32 text;
    int textLen = 0;
    bool moved = false;
    if (!isText() || !isVisible())
        return false;
    node = getNode();
    text = node->getText();
    textLen = text.length();
    if (_data->getOffset() >= textLen)
        return false;
    // skip spaces
    while (_data->getOffset() < textLen && IsUnicodeSpace(text[_data->getOffset()])) {
        _data->addOffset(1);
        //moved = true;
    }
    // skip non-spaces
    while (_data->getOffset() < textLen) {
        if (IsUnicodeSpace(text[_data->getOffset()]))
            break;
        moved = true;
        _data->addOffset(1);
    }
    return moved;
}

/// move to next visible word end (in sentence)
bool ldomXPointerEx::nextVisibleWordEndInSentence(bool thisBlockOnly) {
    if (isNull())
        return false;
    ldomNode* node = NULL;
    lString32 text;
    int textLen = 0;
    //bool moved = false;
    for (;;) {
        if (!isText() || !isVisible()) {
            // move to previous text
            if (!nextVisibleText(thisBlockOnly))
                return false;
            node = getNode();
            text = node->getText();
            textLen = text.length();
            _data->setOffset(0);
            //moved = true;
        } else {
            for (;;) {
                node = getNode();
                text = node->getText();
                textLen = text.length();
                if (_data->getOffset() < textLen)
                    break;
                if (!nextVisibleText(thisBlockOnly))
                    return false;
                _data->setOffset(0);
            }
        }
        bool nonSpaceFound = false;
        // skip non-spaces
        while (_data->getOffset() < textLen) {
            if (IsUnicodeSpace(text[_data->getOffset()]))
                break;
            nonSpaceFound = true;
            _data->addOffset(1);
            if (canWrapWordAfter(text[_data->getOffset()])) // We moved to a CJK char
                return true;
        }
        if (nonSpaceFound)
            return true;
        // skip spaces
        while (_data->getOffset() < textLen && IsUnicodeSpace(text[_data->getOffset()])) {
            _data->addOffset(1);
            //moved = true;
        }
        // skip non-spaces
        while (_data->getOffset() < textLen) {
            if (IsUnicodeSpace(text[_data->getOffset()]))
                break;
            nonSpaceFound = true;
            _data->addOffset(1);
            if (canWrapWordAfter(text[_data->getOffset()])) // We moved to a CJK char
                return true;
        }
        if (nonSpaceFound)
            return true;
    }
}

/// move to previous visible word end (in sentence)
bool ldomXPointerEx::prevVisibleWordEndInSentence(bool thisBlockOnly) {
    if (isNull())
        return false;
    ldomNode* node = NULL;
    lString32 text;
    bool moved = false;
    for (;;) {
        if (!isText() || !isVisible() || _data->getOffset() == 0) {
            // move to previous text
            if (!prevVisibleText(thisBlockOnly))
                return false;
            node = getNode();
            text = node->getText();
            int textLen = text.length();
            _data->setOffset(textLen);
            moved = true;
        } else {
            node = getNode();
            text = node->getText();
        }
        // skip spaces
        while (_data->getOffset() > 0 && IsUnicodeSpace(text[_data->getOffset() - 1])) {
            _data->addOffset(-1);
            moved = true;
        }
        if (moved && _data->getOffset() > 0)
            return true; // found!
        // skip non-spaces
        while (_data->getOffset() > 0) {
            if (IsUnicodeSpace(text[_data->getOffset() - 1]))
                break;
            if (moved && canWrapWordAfter(text[_data->getOffset()])) // We moved to a CJK char
                return true;
            moved = true;
            _data->addOffset(-1);
        }
        // skip spaces
        while (_data->getOffset() > 0 && IsUnicodeSpace(text[_data->getOffset() - 1])) {
            _data->addOffset(-1);
            moved = true;
        }
        if (moved && _data->getOffset() > 0)
            return true; // found!
    }
}

/// returns true if current position is visible word beginning
bool ldomXPointerEx::isVisibleWordStart() {
    if (isNull())
        return false;
    if (!isText() || !isVisible())
        return false;
    ldomNode* node = getNode();
    lString32 text = node->getText();
    int textLen = text.length();
    int i = _data->getOffset();
    // We're actually testing the boundary between the char at i-1 and
    // the char at i. So, we return true when [i] is the first letter
    // of a word.
    lChar32 currCh = i < textLen ? text[i] : 0;
    lChar32 prevCh = i <= textLen && i > 0 ? text[i - 1] : 0;
    if (canWrapWordBefore(currCh)) {
        // If [i] is a CJK char (that's what canWrapWordBefore()
        // checks), this is a visible word start.
        return true;
    }
    if (IsWordSeparatorOrNull(prevCh) && !IsWordSeparator(currCh)) {
        // If [i-1] is a space or punctuation (or [i] is the start of the text
        // node) and [i] is a letter: this is a visible word start.
        return true;
    }
    return false;
}

/// returns true if current position is visible word end
bool ldomXPointerEx::isVisibleWordEnd() {
    if (isNull())
        return false;
    if (!isText() || !isVisible())
        return false;
    ldomNode* node = getNode();
    lString32 text = node->getText();
    int textLen = text.length();
    int i = _data->getOffset();
    // We're actually testing the boundary between the char at i-1 and
    // the char at i. So, we return true when [i-1] is the last letter
    // of a word.
    lChar32 currCh = i > 0 ? text[i - 1] : 0;
    lChar32 nextCh = i < textLen ? text[i] : 0;
    if (canWrapWordAfter(currCh)) {
        // If [i-1] is a CJK char (that's what canWrapWordAfter()
        // checks), this is a visible word end.
        return true;
    }
    if (!IsWordSeparator(currCh) && IsWordSeparatorOrNull(nextCh)) {
        // If [i-1] is a letter and [i] is a space or punctuation (or [i-1] is
        // the last letter of a text node): this is a visible word end.
        return true;
    }
    return false;
}

/// returns block owner node of current node (or current node if it's block)
ldomNode* ldomXPointerEx::getThisBlockNode() {
    if (isNull())
        return NULL;
    ldomNode* node = getNode();
    if (node->isText())
        node = node->getParentNode();
    for (;;) {
        if (!node)
            return NULL;
        lvdom_element_render_method rm = node->getRendMethod();
        switch (rm) {
            case erm_block:
            case erm_final:
            case erm_table:
            case erm_table_row_group:
            case erm_table_row:
                return node;
            default:
                break; // ignore
        }
        node = node->getParentNode();
    }
}

/// returns true if points to last visible text inside block element
bool ldomXPointerEx::isLastVisibleTextInBlock() {
    if (!isText())
        return false;
    ldomXPointerEx pos(*this);
    return !pos.nextVisibleText(true);
}

/// returns true if points to first visible text inside block element
bool ldomXPointerEx::isFirstVisibleTextInBlock() {
    if (!isText())
        return false;
    ldomXPointerEx pos(*this);
    return !pos.prevVisibleText(true);
}

// sentence navigation

/// returns true if points to beginning of sentence
bool ldomXPointerEx::isSentenceStart() {
    if (isNull())
        return false;
    if (!isText() || !isVisible())
        return false;
    ldomNode* node = getNode();
    lString32 text = node->getText();
    int textLen = text.length();
    int i = _data->getOffset();
    lChar32 currCh = i < textLen ? text[i] : 0;
    lChar32 prevCh = i > 0 ? text[i - 1] : 0;
    lChar32 prevPrevNonSpace = 0;
    lChar32 prevNonSpace = 0;
    int prevNonSpace_i = -1;
    for (; i > 0; i--) {
        lChar32 ch = text[i - 1];
        if (!IsUnicodeSpace(ch)) {
            prevNonSpace = ch;
            prevNonSpace_i = i - 1;
            break;
        }
    }
    if (prevNonSpace) {
        for (i = prevNonSpace_i; i > 0; i--) {
            lChar32 ch = text[i - 1];
            if (!IsUnicodeSpace(ch)) {
                prevPrevNonSpace = ch;
                break;
            }
        }
    }
    if (!prevNonSpace) {
        ldomXPointerEx pos(*this);
        while (!prevNonSpace && pos.prevVisibleText(true)) {
            lString32 prevText = pos.getText();
            for (int j = prevText.length() - 1; j >= 0; j--) {
                lChar32 ch = prevText[j];
                if (!IsUnicodeSpace(ch)) {
                    prevNonSpace = ch;
                    for (int k = j; k > 0; k--) {
                        ch = prevText[k - 1];
                        if (!IsUnicodeSpace(ch)) {
                            prevPrevNonSpace = ch;
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }

    // skip separated separator.
    if (1 == textLen) {
        switch (currCh) {
            case '.':
            case '?':
            case '!':
            case U'\x2026': // horizontal ellipsis
                return false;
        }
    }

    if (!IsUnicodeSpace(currCh) && IsUnicodeSpaceOrNull(prevCh)) {
        switch (prevNonSpace) {
            case 0:
            case '.':
            case '?':
            case '!':
            case U'\x2026': // horizontal ellipsis
                return true;
            case '"':       // QUOTATION MARK
            case U'\x201d': // RIGHT DOUBLE QUOTATION MARK
                switch (prevPrevNonSpace) {
                    case '.':
                    case '?':
                    case '!':
                    case U'\x2026': // horizontal ellipsis
                        return true;
                }
                break;
            default:
                return false;
        }
    }
    return false;
}

/// returns true if points to end of sentence
bool ldomXPointerEx::isSentenceEnd() {
    if (isNull())
        return false;
    if (!isText() || !isVisible())
        return false;
    ldomNode* node = getNode();
    lString32 text = node->getText();
    int textLen = text.length();
    int i = _data->getOffset();
    lChar32 currCh = i < textLen ? text[i] : 0;
    lChar32 prevCh = i > 0 ? text[i - 1] : 0;
    lChar32 prevPrevCh = i > 1 ? text[i - 2] : 0;
    if (IsUnicodeSpaceOrNull(currCh)) {
        switch (prevCh) {
            case 0:
            case '.':
            case '?':
            case '!':
            case U'\x2026': // horizontal ellipsis
                return true;
            case '"':
            case U'\x201d': // RIGHT DOUBLE QUOTATION MARK
                switch (prevPrevCh) {
                    case '.':
                    case '?':
                    case '!':
                    case U'\x2026': // horizontal ellipsis
                        return true;
                }
                break;
            default:
                break;
        }
    }
    // word is not ended with . ! ?
    // check whether it's last word of block
    ldomXPointerEx pos(*this);
    return !pos.nextVisibleWordStartInSentence(false);
    //return !pos.thisVisibleWordEndInSentence();
}

/// move to beginning of current visible text sentence
bool ldomXPointerEx::thisSentenceStart() {
    if (isNull())
        return false;
    if (!isText() && !nextVisibleText() && !prevVisibleText())
        return false;
    for (;;) {
        if (isSentenceStart())
            return true;
        if (!prevVisibleWordStartInSentence(true))
            return false;
    }
}

/// move to end of current visible text sentence
bool ldomXPointerEx::thisSentenceEnd() {
    if (isNull())
        return false;
    if (!isText() && !nextVisibleText() && !prevVisibleText())
        return false;
    for (;;) {
        if (isSentenceEnd())
            return true;
        if (!nextVisibleWordEndInSentence(true))
            return false;
    }
}

/// move to beginning of next visible text sentence
bool ldomXPointerEx::nextSentenceStart() {
    if (!isSentenceStart() && !thisSentenceEnd())
        return false;
    for (;;) {
        if (!nextVisibleWordStartInSentence(false))
            return false;
        if (isSentenceStart())
            return true;
    }
}

/// move to beginning of prev visible text sentence
bool ldomXPointerEx::prevSentenceStart() {
    if (!thisSentenceStart())
        return false;
    for (;;) {
        if (!prevVisibleWordStartInSentence(false))
            return false;
        if (isSentenceStart())
            return true;
    }
}

/// move to end of next visible text sentence
bool ldomXPointerEx::nextSentenceEnd() {
    if (!nextSentenceStart())
        return false;
    return thisSentenceEnd();
}

/// move to end of next visible text sentence
bool ldomXPointerEx::prevSentenceEnd() {
    if (!thisSentenceStart())
        return false;
    for (;;) {
        if (!prevVisibleWordEndInSentence(false))
            return false;
        if (isSentenceEnd())
            return true;
    }
}

/// backward iteration by elements of DOM three
bool ldomXPointerEx::prevElement() {
    if (!ensureElement())
        return false;
    for (;;) {
        if (prevSiblingElement()) {
            while (lastElementChild())
                ;
            return true;
        }
        if (!parent())
            return false;
        return true;
    }
}

/// move to next final visible node (~paragraph)
bool ldomXPointerEx::nextVisibleFinal() {
    for (;;) {
        if (!nextElement())
            return false;
        if (isVisibleFinal())
            return true;
    }
}

/// move to previous final visible node (~paragraph)
bool ldomXPointerEx::prevVisibleFinal() {
    for (;;) {
        if (!prevElement())
            return false;
        if (isVisibleFinal())
            return true;
    }
}
