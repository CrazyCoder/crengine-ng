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

#include <lvtocitem.h>
#include <ldomdocument.h>
#include <crlog.h>

void LVTocItem::addChild(LVTocItem* item) {
    item->_level = _level + 1;
    item->_parent = this;
    item->_index = _children.length(), item->_doc = _doc;
    _children.add(item);
}

/// returns position pointer
ldomXPointer LVTocItem::getXPointer() {
    if (_position.isNull() && !_path.empty()) {
        _position = _doc->createXPointer(_path);
        if (_position.isNull()) {
            CRLog::trace("TOC node is not found for path %s", LCSTR(_path));
        } else {
            CRLog::trace("TOC node is found for path %s", LCSTR(_path));
            // CRLog::trace("           gives xpointer: %s", UnicodeToLocal(_position.toString()).c_str());
        }
    }
    return _position;
}

/// returns position path
lString32 LVTocItem::getPath() {
    if (_path.empty() && !_position.isNull())
        _path = _position.toString();
    return _path;
}

/// returns Y position
int LVTocItem::getY() {
#if BUILD_LITE != 1
    return getXPointer().toPoint().y;
#else
    return 0;
#endif
}

/// serialize to byte array (pointer will be incremented by number of bytes written)
bool LVTocItem::serialize(SerialBuf& buf) {
    //    LVTocItem *     _parent;
    //    int             _level;
    //    int             _index;
    //    int             _page;
    //    int             _percent;
    //    lString32       _name;
    //    ldomXPointer    _position;
    //    LVPtrVector<LVTocItem> _children;

    buf << (lUInt32)_level << (lUInt32)_index << (lUInt32)_page << (lUInt32)_percent << (lUInt32)_children.length() << _name << getPath();
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
bool LVTocItem::deserialize(ldomDocument* doc, SerialBuf& buf) {
    if (buf.error())
        return false;
    lInt32 childCount = 0;
    buf >> _level >> _index >> _page >> _percent >> childCount >> _name >> _path;
    //    CRLog::trace("[%d] %05d  %s  %s", _level, _page, LCSTR(_name), LCSTR(_path));
    if (buf.error())
        return false;
    //    if ( _level>0 ) {
    //        _position = doc->createXPointer( _path );
    //        if ( _position.isNull() ) {
    //            CRLog::error("Cannot find TOC node by path %s", LCSTR(_path) );
    //            buf.seterror();
    //            return false;
    //        }
    //    }
    for (int i = 0; i < childCount; i++) {
        LVTocItem* item = new LVTocItem(doc);
        if (!item->deserialize(doc, buf)) {
            delete item;
            return false;
        }
        item->_parent = this;
        _children.add(item);
        if (buf.error())
            return false;
    }
    return true;
}

/// returns page number
//int LVTocItem::getPageNum( LVRendPageList & pages )
//{
//    return getSectionPage( _position.getNode(), pages );
//}
