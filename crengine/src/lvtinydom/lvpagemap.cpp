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

#include <lvpagemap.h>
#include <ldomdocument.h>
#include <crlog.h>

/// returns position pointer
ldomXPointer LVPageMapItem::getXPointer() {
    if (_position.isNull() && !_path.empty()) {
        _position = _doc->createXPointer(_path);
        if (_position.isNull()) {
            CRLog::trace("LVPageMapItem node is not found for path %s", LCSTR(_path));
        } else {
            CRLog::trace("LVPageMapItem node is found for path %s", LCSTR(_path));
        }
    }
    return _position;
}

/// returns position path
lString32 LVPageMapItem::getPath() {
    if (_path.empty() && !_position.isNull())
        _path = _position.toString();
    return _path;
}

/// returns Y position
int LVPageMapItem::getDocY(bool refresh) {
    if (_doc_y < 0 || refresh)
        _doc_y = getXPointer().toPoint().y;
    if (_doc_y < 0 && !_position.isNull()) {
        // We got a xpointer, that did not resolve to a point.
        // It may be because the node it points to is invisible,
        // which may happen with pagebreak spans (that may not
        // be empty, and were set to "display: none").
        ldomXPointerEx xp = _position;
        if (!xp.isVisible()) {
            if (xp.nextVisibleText()) {
                _doc_y = xp.toPoint().y;
            } else {
                xp = _position;
                if (xp.prevVisibleText()) {
                    _doc_y = xp.toPoint().y;
                }
            }
        }
    }
    return _doc_y;
}

/// serialize to byte array (pointer will be incremented by number of bytes written)
bool LVPageMapItem::serialize(SerialBuf& buf) {
    buf << (lUInt32)_index << (lUInt32)_page << (lUInt32)_doc_y << _label << getPath();
    return !buf.error();
}

/// deserialize from byte array (pointer will be incremented by number of bytes read)
bool LVPageMapItem::deserialize(ldomDocument* doc, SerialBuf& buf) {
    if (buf.error())
        return false;
    buf >> _index >> _page >> _doc_y >> _label >> _path;
    return !buf.error();
}

/// serialize to byte array (pointer will be incremented by number of bytes written)
bool LVPageMap::serialize(SerialBuf& buf) {
    buf << (lUInt32)_page_info_valid << (lUInt32)_children.length() << _source;
    if (buf.error())
        return false;
    for (int i = 0; i < _children.length(); i++) {
        _children[i]->serialize(buf);
        if (buf.error())
            return false;
    }
    return !buf.error();
}

/// deserialize from byte array (pointer will be incremented by number of bytes read)
bool LVPageMap::deserialize(ldomDocument* doc, SerialBuf& buf) {
    if (buf.error())
        return false;
    lUInt32 childCount = 0;
    lUInt32 pageInfoValid = 0;
    buf >> pageInfoValid >> childCount >> _source;
    if (buf.error())
        return false;
    _page_info_valid = (bool)pageInfoValid;
    for (lUInt32 i = 0; i < childCount; i++) {
        LVPageMapItem* item = new LVPageMapItem(doc);
        if (!item->deserialize(doc, buf)) {
            delete item;
            return false;
        }
        _children.add(item);
        if (buf.error())
            return false;
    }
    return true;
}
